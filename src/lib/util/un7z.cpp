// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    un7z.cpp

    Functions to manipulate data within 7z files, backed by the r7z
    archive reader from libretro-common instead of the LZMA SDK.

    r7z borrows the whole archive image from memory, so the file is
    read in once at open.  The SDK's per-folder output cache is gone
    with it: extracting several entries from one solid folder decodes
    the folder once per entry rather than once, which is a fair trade
    for ROM loading, where each entry is extracted exactly once.

***************************************************************************/

#include "unzip.h"

#include "corestr.h"
#include "ioprocs.h"
#include "unicode.h"

#include "osdcore.h"
#include "osdfile.h"

#include <7z/r7z_archive.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <utility>
#include <vector>


namespace util {

namespace {

/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

class m7z_file_impl
{
public:
	typedef std::unique_ptr<m7z_file_impl> ptr;

	m7z_file_impl(std::string &&filename) noexcept
		: m_filename(std::move(filename))
	{
	}

	m7z_file_impl(random_read::ptr &&file) noexcept
		: m7z_file_impl(std::string())
	{
		m_file = std::move(file);
	}

	~m7z_file_impl()
	{
		if (m_archive)
			r7z_archive_close(m_archive);
	}

	static ptr find_cached(std::string_view filename) noexcept
	{
		std::lock_guard<std::mutex> guard(s_cache_mutex);
		for (std::size_t cachenum = 0; cachenum < s_cache.size(); cachenum++)
		{
			// if we have a valid entry and it matches our filename, use it and remove from the cache
			if (s_cache[cachenum] && (filename == s_cache[cachenum]->m_filename))
			{
				using std::swap;
				ptr result;
				swap(s_cache[cachenum], result);
				osd_printf_verbose("un7z: found %s in cache\n", filename);
				return result;
			}
		}
		return ptr();
	}

	static void close(ptr &&archive) noexcept
	{
		if (!archive)
			return;

		// archives without a filename can't be cached
		if (archive->m_filename.empty())
			return;

		// reset the position and put it back in the cache
		std::lock_guard<std::mutex> guard(s_cache_mutex);
		archive->m_curr_file_idx = -1;
		for (auto &cached : s_cache)
		{
			if (!cached)
			{
				cached = std::move(archive);
				return;
			}
		}
	}

	static void cache_clear() noexcept
	{
		// clear call cache entries
		std::lock_guard<std::mutex> guard(s_cache_mutex);
		for (auto &cached : s_cache)
			cached.reset();
	}

	std::error_condition initialize() noexcept;

	int first_file() noexcept
	{
		return search(0, 0, std::string_view(), false, false, false);
	}

	int next_file() noexcept
	{
		return (m_curr_file_idx < 0) ? -1 : search(m_curr_file_idx + 1, 0, std::string_view(), false, false, false);
	}

	int search(std::uint32_t crc) noexcept
	{
		return search(0, crc, std::string_view(), true, false, false);
	}

	int search(std::string_view filename, bool partialpath) noexcept
	{
		return search(0, 0, filename, false, true, partialpath);
	}

	int search(std::uint32_t crc, std::string_view filename, bool partialpath) noexcept
	{
		return search(0, crc, filename, true, true, partialpath);
	}

	bool current_is_directory() const noexcept { return m_curr_is_dir; }
	const std::string &current_name() const noexcept { return m_curr_name; }
	std::uint64_t current_uncompressed_length() const noexcept { return m_curr_length; }
	std::chrono::system_clock::time_point current_last_modified() const noexcept { return m_curr_modified; }
	std::uint32_t current_crc() const noexcept { return m_curr_crc; }

	std::error_condition decompress(void *buffer, std::size_t length) noexcept;

private:
	m7z_file_impl(const m7z_file_impl &) = delete;
	m7z_file_impl(m7z_file_impl &&) = delete;
	m7z_file_impl &operator=(const m7z_file_impl &) = delete;
	m7z_file_impl &operator=(m7z_file_impl &&) = delete;

	int search(
			int i,
			std::uint32_t search_crc,
			std::string_view search_filename,
			bool matchcrc,
			bool matchname,
			bool partialpath) noexcept;
	void make_utf8_name(int index);
	std::error_condition map_r7z_error(int err) const noexcept
	{
		switch (err)
		{
		case R7Z_OK:                return std::error_condition();
		case R7Z_ERROR_MEM:         return std::errc::not_enough_memory;
		case R7Z_ERROR_UNSUPPORTED: return archive_file::error::UNSUPPORTED;
		case R7Z_ERROR_DATA:        return archive_file::error::BAD_SIGNATURE;
		case R7Z_ERROR_CRC:         return archive_file::error::DECOMPRESS_ERROR;
		default:                    return archive_file::error::BAD_SIGNATURE;
		}
	}

	static constexpr std::size_t            CACHE_SIZE = 8;
	static std::array<ptr, CACHE_SIZE>      s_cache;
	static std::mutex                       s_cache_mutex;

	const std::string                       m_filename;             // copy of the 7Z filename (for caching)
	random_read::ptr                        m_file;                 // source file, held until slurped
	std::vector<std::uint8_t>               m_image;                // the whole archive, borrowed by r7z
	r7z_archive_t *                         m_archive = nullptr;

	int                                     m_curr_file_idx = -1;   // current file index
	bool                                    m_curr_is_dir = false;  // current file is directory
	std::string                             m_curr_name;            // current file name
	std::uint64_t                           m_curr_length = 0;      // current file uncompressed length
	std::chrono::system_clock::time_point   m_curr_modified;        // current file modification time
	std::uint32_t                           m_curr_crc = 0;         // current file crc

	std::vector<char32_t>                   m_uchar_buf;            // decoded unicode characters
	std::vector<char>                       m_utf8_buf;             // UTF-8 encoded name
};


class m7z_file_wrapper : public archive_file
{
public:
	m7z_file_wrapper(m7z_file_impl::ptr &&impl) noexcept : m_impl(std::move(impl)) { assert(m_impl); }
	virtual ~m7z_file_wrapper() override { m7z_file_impl::close(std::move(m_impl)); }

	virtual int first_file() noexcept override { return m_impl->first_file(); }
	virtual int next_file() noexcept override { return m_impl->next_file(); }

	virtual int search(std::uint32_t crc) noexcept override
	{
		return m_impl->search(crc);
	}
	virtual int search(std::string_view filename, bool partialpath) noexcept override
	{
		return m_impl->search(filename, partialpath);
	}
	virtual int search(std::uint32_t crc, std::string_view filename, bool partialpath) noexcept override
	{
		return m_impl->search(crc, filename, partialpath);
	}

	virtual bool current_is_directory() const noexcept override { return m_impl->current_is_directory(); }
	virtual const std::string &current_name() const noexcept override { return m_impl->current_name(); }
	virtual std::uint64_t current_uncompressed_length() const noexcept override { return m_impl->current_uncompressed_length(); }
	virtual std::chrono::system_clock::time_point current_last_modified() const noexcept override { return m_impl->current_last_modified(); }
	virtual std::uint32_t current_crc() const noexcept override { return m_impl->current_crc(); }

	virtual std::error_condition decompress(void *buffer, std::size_t length) noexcept override { return m_impl->decompress(buffer, length); }

private:
	m7z_file_impl::ptr m_impl;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

std::array<m7z_file_impl::ptr, m7z_file_impl::CACHE_SIZE> m7z_file_impl::s_cache;
std::mutex m7z_file_impl::s_cache_mutex;



/***************************************************************************
    7Z FILE ACCESS
***************************************************************************/

std::error_condition m7z_file_impl::initialize() noexcept
{
	try
	{
		// open the source file if we were given a name rather than a stream;
		// osd_file reports the size at open, and its random_read adapter
		// cannot answer length(), so capture it here
		std::uint64_t length = 0;
		if (!m_file)
		{
			osd_file::ptr file;
			std::error_condition const err = osd_file::open(m_filename, OPEN_FLAG_READ, file, length);
			if (err)
				return err;
			m_file = osd_file_read(std::move(file));
		}
		else
		{
			std::error_condition const err = m_file->length(length);
			if (err)
				return err;
		}

		// r7z borrows the whole image, so read the file in
		m_image.resize(std::size_t(length));
		std::size_t actual = 0;
		std::error_condition const err = m_file->read_at(0, m_image.data(), m_image.size(), actual);
		if (err)
			return err;
		if (actual != m_image.size())
			return archive_file::error::FILE_TRUNCATED;
		m_file.reset();

		int const r7zerr = r7z_archive_open(&m_archive, m_image.data(), m_image.size());
		if (r7zerr != R7Z_OK)
			return map_r7z_error(r7zerr);

		return std::error_condition();
	}
	catch (std::bad_alloc const &)
	{
		return std::errc::not_enough_memory;
	}
	catch (...)
	{
		return archive_file::error::UNSUPPORTED;
	}
}


std::error_condition m7z_file_impl::decompress(void *buffer, std::size_t length) noexcept
{
	// if we don't have enough buffer, error
	if (length < m_curr_length)
	{
		osd_printf_error("un7z: buffer too small to decompress %s from %s\n", m_curr_name, m_filename);
		return archive_file::error::BUFFER_TOO_SMALL;
	}
	if (m_curr_file_idx < 0)
		return archive_file::error::UNSUPPORTED;

	std::uint8_t *out = nullptr;
	std::size_t out_len = 0;
	int const err = r7z_archive_extract(m_archive, std::uint32_t(m_curr_file_idx), &out, &out_len);
	if (err != R7Z_OK)
	{
		osd_printf_error("un7z: error decompressing %s from %s (%d)\n", m_curr_name, m_filename, err);
		return map_r7z_error(err);
	}

	// copy to destination buffer
	std::memcpy(buffer, out, (std::min<std::size_t>)(length, out_len));
	std::free(out);
	return std::error_condition();
}


int m7z_file_impl::search(
		int i,
		std::uint32_t search_crc,
		std::string_view search_filename,
		bool matchcrc,
		bool matchname,
		bool partialpath) noexcept
{
	try
	{
		int const count = int(r7z_archive_num_entries(m_archive));
		for ( ; i < count; i++)
		{
			const r7z_entry_t *entry = r7z_archive_entry(m_archive, std::uint32_t(i));
			if (!entry)
				break;
			make_utf8_name(i);
			bool const is_dir(entry->is_dir != 0);
			const std::uint64_t size(entry->size);
			const std::uint32_t crc(entry->crc);

			const bool crcmatch(entry->has_crc && (crc == search_crc));
			bool found;
			if (!matchname)
			{
				found = !matchcrc || (crcmatch && !is_dir);
			}
			else
			{
				auto const partialoffset = m_utf8_buf.size() - search_filename.length();
				const bool namematch =
						(search_filename.length() == m_utf8_buf.size()) &&
						(search_filename.empty() || !core_strnicmp(&search_filename[0], &m_utf8_buf[0], search_filename.length()));
				bool const partialmatch =
						partialpath &&
						((m_utf8_buf.size() > search_filename.length()) && (m_utf8_buf[partialoffset - 1] == '/')) &&
						(search_filename.empty() || !core_strnicmp(&search_filename[0], &m_utf8_buf[partialoffset], search_filename.length()));
				found = (!matchcrc || crcmatch) && (namematch || partialmatch);
			}

			if (found)
			{
				// set the name first - resizing it can throw an exception, and we want the state to be consistent
				m_curr_name.assign(m_utf8_buf.begin(), m_utf8_buf.end());
				m_curr_file_idx = i;
				m_curr_is_dir = is_dir;
				m_curr_length = size;
				m_curr_modified = std::chrono::system_clock::from_time_t(std::time_t(0));
				m_curr_crc = crc;

				return i;
			}
		}
	}
	catch (...)
	{
	}
	m_curr_file_idx = -1;
	return -1;
}


void m7z_file_impl::make_utf8_name(int index)
{
	const r7z_entry_t *entry = r7z_archive_entry(m_archive, std::uint32_t(index));

	// NUL-terminated UTF-16LE, length in code units including the NUL
	std::size_t len = 0;
	while (entry->name[len])
		len++;
	len++;

	m_uchar_buf.resize(std::max<std::size_t>(m_uchar_buf.size(), len));
	std::size_t out_pos = 0;
	for (std::size_t in_pos = 0; in_pos < (len - 1); )
	{
		char16_t units[2];
		units[0] = char16_t(entry->name[in_pos]);
		units[1] = ((in_pos + 1) < len) ? char16_t(entry->name[in_pos + 1]) : char16_t(0);
		const int used = uchar_from_utf16(&m_uchar_buf[out_pos], units, len - in_pos);
		if (used < 0)
		{
			in_pos++;
			m_uchar_buf[out_pos++] = 0x00fffd; // Unicode REPLACEMENT CHARACTER
		}
		else
		{
			assert(used > 0);
			in_pos += used;
			out_pos++;
		}
	}
	len = out_pos;

	m_utf8_buf.resize((std::max<std::size_t>)(m_utf8_buf.size(), 4 * len));
	out_pos = 0;
	for (std::size_t in_pos = 0; in_pos < len; in_pos++)
	{
		int produced = utf8_from_uchar(&m_utf8_buf[out_pos], m_utf8_buf.size() - out_pos, m_uchar_buf[in_pos]);
		if (produced < 0)
			produced = utf8_from_uchar(&m_utf8_buf[out_pos], m_utf8_buf.size() - out_pos, 0x00fffd);
		if (produced >= 0)
			out_pos += produced;
	}

	m_utf8_buf.resize(out_pos);
}

} // anonymous namespace


std::error_condition archive_file::open_7z(std::string_view filename, ptr &result) noexcept
{
	// ensure we start with a nullptr result
	result.reset();

	// see if we are in the cache, and reopen if so
	m7z_file_impl::ptr newimpl(m7z_file_impl::find_cached(filename));

	if (!newimpl)
	{
		// allocate memory for the 7z file structure
		try { newimpl = std::make_unique<m7z_file_impl>(std::string(filename)); }
		catch (...) { return std::errc::not_enough_memory; }
		auto const err = newimpl->initialize();
		if (err)
			return err;
	}

	// allocate the archive API wrapper
	result.reset(new (std::nothrow) m7z_file_wrapper(std::move(newimpl)));
	if (result)
	{
		return std::error_condition();
	}
	else
	{
		m7z_file_impl::close(std::move(newimpl));
		return std::errc::not_enough_memory;
	}
}

std::error_condition archive_file::open_7z(random_read::ptr &&file, ptr &result) noexcept
{
	// ensure we start with a nullptr result
	result.reset();

	// allocate memory for the zip_file structure
	m7z_file_impl::ptr newimpl(new (std::nothrow) m7z_file_impl(std::move(file)));
	if (!newimpl)
		return std::errc::not_enough_memory;
	auto const err = newimpl->initialize();
	if (err)
		return err;

	// allocate the archive API wrapper
	result.reset(new (std::nothrow) m7z_file_wrapper(std::move(newimpl)));
	if (result)
	{
		return std::error_condition();
	}
	else
	{
		m7z_file_impl::close(std::move(newimpl));
		return std::errc::not_enough_memory;
	}
}


/*-------------------------------------------------
    _7z_file_cache_clear - clear the _7Z file
    cache and free all memory
-------------------------------------------------*/

void m7z_file_cache_clear() noexcept
{
	// This is a trampoline called from unzip.cpp to avoid the need to have the zip and 7zip code in one file
	m7z_file_impl::cache_clear();
}

} // namespace util
