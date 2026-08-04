// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    chd.cpp

    MAME Compressed Hunks of Data file format, read side, implemented
    over the rchd reader from libretro-common.

    rchd performs no I/O of its own: it describes every byte range it
    needs as an (offset, length, source) request, and this adapter
    satisfies each request from the underlying util::random_read (or the
    parent chd_file's, when the request names the parent image).  The
    old zlib/LZMA-SDK/libFLAC codec stack is gone; codec selection and
    decompression live inside rchd, which also handles zstd-compressed
    images the old stack could not.

***************************************************************************/

#include "chd.h"

#include "corefile.h"

#include <formats/rchd.h>

#include <cstring>


//**************************************************************************
//  CONSTANTS
//**************************************************************************

const char *HARD_DISK_METADATA_FORMAT = "CYLS:%d,HEADS:%d,SECS:%d,BPS:%d";
const char *CDROM_TRACK_METADATA_FORMAT = "TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d";
const char *CDROM_TRACK_METADATA2_FORMAT = "TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d PREGAP:%d PGTYPE:%s PGSUB:%s POSTGAP:%d";
const char *GDROM_TRACK_METADATA_FORMAT = "TRACK:%d TYPE:%s SUBTYPE:%s FRAMES:%d PAD:%d PREGAP:%d PGTYPE:%s PGSUB:%s POSTGAP:%d";
const char *AV_METADATA_FORMAT = "FPS:%d.%06d WIDTH:%d HEIGHT:%d INTERLACED:%d CHANNELS:%d SAMPLERATE:%d";


namespace {

// map an rchd error to the chd_file error space
std::error_condition rchd_to_chd(int err)
{
	switch (err)
	{
	case RCHD_OK:                 return std::error_condition();
	case RCHD_ERROR_DATA:         return chd_file::error::INVALID_DATA;
	case RCHD_ERROR_PARAM:        return chd_file::error::INVALID_DATA;
	case RCHD_ERROR_MEM:          return std::errc::not_enough_memory;
	case RCHD_ERROR_UNSUPPORTED:  return chd_file::error::UNSUPPORTED_FORMAT;
	case RCHD_ERROR_CRC:          return chd_file::error::DECOMPRESSION_ERROR;
	case RCHD_ERROR_NO_PARENT:    return chd_file::error::REQUIRES_PARENT;
	case RCHD_ERROR_STATE:        return chd_file::error::INVALID_STATE;
	default:                      return chd_file::error::INVALID_DATA;
	}
}

util::sha1_t sha1_from_bytes(const uint8_t *bytes)
{
	util::sha1_t result;
	std::memcpy(result.m_raw, bytes, sizeof(result.m_raw));
	return result;
}

} // anonymous namespace


//**************************************************************************
//  CHD FILE MANAGEMENT
//**************************************************************************

chd_file::chd_file()
	: m_chd(nullptr)
	, m_parent(nullptr)
{
}

chd_file::~chd_file()
{
	close();
}

/**
 * @brief   Satisfy one rchd byte-range request from the right file.
 */
std::error_condition chd_file::feed_request(const rchd_request *req)
{
	// resolve which file the request names
	util::random_read *src = nullptr;
	if (req->source == RCHD_SOURCE_SELF)
		src = m_file.get();
	else if (m_parent)
		src = m_parent->m_file.get();
	if (!src)
		return error::REQUIRES_PARENT;

	// a request is bounded by the hunk size, so a stack-side buffer
	// would do, but the length is not compile-time bounded; keep it heap
	uint8_t stackbuf[4096];
	std::unique_ptr<uint8_t []> heapbuf;
	uint8_t *buf = stackbuf;
	if (req->length > sizeof(stackbuf))
	{
		heapbuf.reset(new (std::nothrow) uint8_t [req->length]);
		if (!heapbuf)
			return std::errc::not_enough_memory;
		buf = heapbuf.get();
	}

	size_t actual = 0;
	std::error_condition ioerr = src->read_at(req->offset, buf, req->length, actual);
	if (ioerr)
		return ioerr;
	if (actual == 0)
		return error::INVALID_DATA; // EOF where the map said bytes are

	int err = rchd_feed(m_chd, buf, actual);
	if (err != RCHD_OK)
		return rchd_to_chd(err);
	return std::error_condition();
}

std::error_condition chd_file::open_common(chd_file *parent)
{
	m_parent = parent;

	m_chd = rchd_new();
	if (!m_chd)
		return std::errc::not_enough_memory;

	// pump the open sequence
	rchd_request req;
	for (;;)
	{
		int err = rchd_open_step(m_chd, &req);
		if (err == RCHD_OK)
			break;
		if (err != RCHD_PENDING)
		{
			close();
			return rchd_to_chd(err);
		}
		std::error_condition ferr = feed_request(&req);
		if (ferr)
		{
			close();
			return ferr;
		}
	}

	// bind the parent if the image differences against one
	const rchd_info_t *info = rchd_info(m_chd);
	if (info->has_parent)
	{
		if (!m_parent || !m_parent->opened())
		{
			close();
			return error::REQUIRES_PARENT;
		}
		if (!rchd_parent_sha1_matches(m_chd, m_parent->sha1().m_raw))
		{
			close();
			return error::INVALID_PARENT;
		}
		int err = rchd_set_parent(m_chd, m_parent->m_chd);
		if (err != RCHD_OK)
		{
			close();
			return rchd_to_chd(err);
		}
	}
	return std::error_condition();
}

std::error_condition chd_file::open(std::string_view filename, bool writeable, chd_file *parent)
{
	// the read-only build refuses writable opens rather than quietly
	// downgrading them; a caller that wants to write would corrupt state
	if (writeable)
		return error::FILE_NOT_WRITEABLE;

	util::core_file::ptr file;
	std::error_condition filerr = util::core_file::open(filename, OPEN_FLAG_READ, file);
	if (filerr)
		return filerr;
	return open(std::move(file), false, parent);
}

std::error_condition chd_file::open(util::random_read_write::ptr &&file, bool writeable, chd_file *parent)
{
	if (m_chd)
		return error::ALREADY_OPEN;
	if (!file)
		return error::INVALID_FILE;
	if (writeable)
		return error::FILE_NOT_WRITEABLE;

	m_file = std::move(file);
	std::error_condition err = open_common(parent);
	if (err)
		m_file.reset();
	return err;
}

void chd_file::close()
{
	if (m_chd)
	{
		rchd_free(m_chd);
		m_chd = nullptr;
	}
	m_file.reset();
	m_parent = nullptr;
}


//**************************************************************************
//  GETTERS
//**************************************************************************

util::random_read &chd_file::file()
{
	return *m_file;
}

uint32_t chd_file::version() const
{
	return m_chd ? rchd_info(m_chd)->version : 0;
}

uint64_t chd_file::logical_bytes() const
{
	return m_chd ? rchd_info(m_chd)->logical_bytes : 0;
}

uint32_t chd_file::hunk_bytes() const
{
	return m_chd ? rchd_info(m_chd)->hunk_bytes : 0;
}

uint32_t chd_file::hunk_count() const
{
	return m_chd ? rchd_info(m_chd)->hunk_count : 0;
}

uint32_t chd_file::unit_bytes() const
{
	return m_chd ? rchd_info(m_chd)->unit_bytes : 0;
}

uint64_t chd_file::unit_count() const
{
	if (!m_chd)
		return 0;
	const rchd_info_t *info = rchd_info(m_chd);
	return info->logical_bytes / info->unit_bytes;
}

bool chd_file::compressed() const
{
	return m_chd && rchd_info(m_chd)->compressors[0] != RCHD_CODEC_NONE;
}

chd_codec_type chd_file::compression(int index) const
{
	if (!m_chd || index < 0 || index >= 4)
		return CHD_CODEC_NONE;
	return rchd_info(m_chd)->compressors[index];
}

util::sha1_t chd_file::sha1()
{
	// V1/V2 images carry MD5s only; the old reader returned null for
	// versions without the field, and the stored bytes as-is otherwise
	if (!m_chd || rchd_info(m_chd)->version < 3)
		return util::sha1_t::null;
	return sha1_from_bytes(rchd_info(m_chd)->sha1);
}

util::sha1_t chd_file::raw_sha1()
{
	// the raw (data-only) SHA-1 first appears in V4
	if (!m_chd || rchd_info(m_chd)->version < 4)
		return util::sha1_t::null;
	return sha1_from_bytes(rchd_info(m_chd)->raw_sha1);
}

util::sha1_t chd_file::parent_sha1()
{
	if (!m_chd || rchd_info(m_chd)->version < 3)
		return util::sha1_t::null;
	return sha1_from_bytes(rchd_info(m_chd)->parent_sha1);
}


//**************************************************************************
//  READING
//**************************************************************************

/**
 * @brief   Arm and pump one byte-range read to completion.
 */
std::error_condition chd_file::pump_read(uint64_t offset, void *buffer, size_t bytes)
{
	if (!m_chd)
		return error::NOT_OPEN;

	int err = rchd_read_begin(m_chd, offset, buffer, bytes);
	if (err != RCHD_OK)
		return rchd_to_chd(err);

	rchd_request req;
	for (;;)
	{
		err = rchd_read_step(m_chd, &req);
		if (err == RCHD_OK)
			return std::error_condition();
		if (err != RCHD_PENDING)
			return rchd_to_chd(err);
		std::error_condition ferr = feed_request(&req);
		if (ferr)
			return ferr;
	}
}

std::error_condition chd_file::read_hunk(uint32_t hunknum, void *buffer)
{
	if (!m_chd)
		return error::NOT_OPEN;
	const rchd_info_t *info = rchd_info(m_chd);
	if (hunknum >= info->hunk_count)
		return error::HUNK_OUT_OF_RANGE;

	// the last hunk of an image whose logical size is not hunk-aligned
	// is still produced whole, matching the old reader
	uint64_t offset = uint64_t(hunknum) * info->hunk_bytes;
	uint64_t remaining = info->logical_bytes - offset;
	size_t length = size_t(std::min<uint64_t>(info->hunk_bytes, remaining));
	std::error_condition err = pump_read(offset, buffer, length);
	if (err)
		return err;
	if (length < info->hunk_bytes)
		std::memset(reinterpret_cast<uint8_t *>(buffer) + length, 0, info->hunk_bytes - length);
	return std::error_condition();
}

std::error_condition chd_file::read_units(uint64_t unitnum, void *buffer, uint32_t count)
{
	return read_bytes(unitnum * unit_bytes(), buffer, count * unit_bytes());
}

std::error_condition chd_file::read_bytes(uint64_t offset, void *buffer, uint32_t bytes)
{
	if (!m_chd)
		return error::NOT_OPEN;
	if (offset + bytes > rchd_info(m_chd)->logical_bytes)
		return error::HUNK_OUT_OF_RANGE;
	return pump_read(offset, buffer, bytes);
}


//**************************************************************************
//  METADATA
//**************************************************************************

const rchd_metadata_t *chd_file::metadata_find(chd_metadata_tag searchtag, uint32_t searchindex) const
{
	if (!m_chd)
		return nullptr;
	if (searchtag == CHDMETATAG_WILDCARD)
		return rchd_metadata(m_chd, searchindex);
	return rchd_metadata_find(m_chd, searchtag, searchindex);
}

std::error_condition chd_file::read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, std::string &output)
{
	const rchd_metadata_t *meta = metadata_find(searchtag, searchindex);
	if (!meta)
		return error::METADATA_NOT_FOUND;
	output.assign(reinterpret_cast<const char *>(meta->data), meta->length);
	return std::error_condition();
}

std::error_condition chd_file::read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, std::vector<uint8_t> &output)
{
	const rchd_metadata_t *meta = metadata_find(searchtag, searchindex);
	if (!meta)
		return error::METADATA_NOT_FOUND;
	output.assign(meta->data, meta->data + meta->length);
	return std::error_condition();
}

std::error_condition chd_file::read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, void *output, uint32_t outputlen, uint32_t &resultlen)
{
	const rchd_metadata_t *meta = metadata_find(searchtag, searchindex);
	if (!meta)
		return error::METADATA_NOT_FOUND;
	resultlen = meta->length;
	std::memcpy(output, meta->data, std::min(outputlen, resultlen));
	return std::error_condition();
}

std::error_condition chd_file::read_metadata(chd_metadata_tag searchtag, uint32_t searchindex, std::vector<uint8_t> &output, chd_metadata_tag &resulttag, uint8_t &resultflags)
{
	const rchd_metadata_t *meta = metadata_find(searchtag, searchindex);
	if (!meta)
		return error::METADATA_NOT_FOUND;
	output.assign(meta->data, meta->data + meta->length);
	resulttag = meta->tag;
	resultflags = meta->flags;
	return std::error_condition();
}


//**************************************************************************
//  WRITE SIDE (unsupported)
//**************************************************************************

// This build reads CHDs; it does not create or modify them.  The only
// in-tree caller of the write side is romload's diff-CHD path, which is
// reached for writable disk regions only -- media this core does not
// have.  Failing with UNSUPPORTED_FORMAT there turns a would-be
// link-time hole into a clean runtime error.

std::error_condition chd_file::create(std::string_view, uint64_t, uint32_t, uint32_t, chd_codec_type [4])
{
	return error::UNSUPPORTED_FORMAT;
}

std::error_condition chd_file::create(util::random_read_write::ptr &&, uint64_t, uint32_t, uint32_t, chd_codec_type [4])
{
	return error::UNSUPPORTED_FORMAT;
}

std::error_condition chd_file::create(std::string_view, uint64_t, uint32_t, chd_codec_type [4], chd_file &)
{
	return error::UNSUPPORTED_FORMAT;
}

std::error_condition chd_file::create(util::random_read_write::ptr &&, uint64_t, uint32_t, chd_codec_type [4], chd_file &)
{
	return error::UNSUPPORTED_FORMAT;
}

std::error_condition chd_file::write_hunk(uint32_t, const void *)
{
	return error::FILE_NOT_WRITEABLE;
}

std::error_condition chd_file::write_units(uint64_t, const void *, uint32_t)
{
	return error::FILE_NOT_WRITEABLE;
}

std::error_condition chd_file::write_bytes(uint64_t, const void *, uint32_t)
{
	return error::FILE_NOT_WRITEABLE;
}

std::error_condition chd_file::write_metadata(chd_metadata_tag, uint32_t, const void *, uint32_t, uint8_t)
{
	return error::FILE_NOT_WRITEABLE;
}

std::error_condition chd_file::delete_metadata(chd_metadata_tag, uint32_t)
{
	return error::FILE_NOT_WRITEABLE;
}

std::error_condition chd_file::clone_all_metadata(chd_file &)
{
	return error::FILE_NOT_WRITEABLE;
}


//**************************************************************************
//  ERROR CATEGORY
//**************************************************************************

std::error_category const &chd_category() noexcept
{
	class chd_category_impl : public std::error_category
	{
		virtual char const *name() const noexcept override { return "chd"; }

		virtual std::string message(int condition) const override
		{
			using namespace std::literals;
			static std::string_view const s_messages[] = {
					"No error"sv,
					"No drive interface"sv,
					"File not open"sv,
					"File already open"sv,
					"Invalid file"sv,
					"Invalid data"sv,
					"Requires parent"sv,
					"File not writeable"sv,
					"Codec error"sv,
					"Invalid parent"sv,
					"Hunk out of range"sv,
					"Decompression error"sv,
					"Compression error"sv,
					"Can't verify file"sv,
					"Can't find metadata"sv,
					"Invalid metadata size"sv,
					"Mismatched DIFF and CHD or unsupported CHD version"sv,
					"Incomplete verify"sv,
					"Invalid metadata"sv,
					"Invalid state"sv,
					"Operation pending"sv,
					"Unsupported format"sv,
					"Unknown compression type"sv,
					"Currently examining parent"sv,
					"Currently compressing"sv };
			if ((0 <= condition) && (std::size(s_messages) > condition))
				return std::string(s_messages[condition]);
			else
				return "Unknown error"s;
		}
	};
	static chd_category_impl const s_chd_category_instance;
	return s_chd_category_instance;
}
