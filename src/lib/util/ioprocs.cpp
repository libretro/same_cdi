// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    ioprocs.h

    I/O interfaces

***************************************************************************/

#include "ioprocs.h"

#include "corefile.h"
#include "ioprocsfill.h"

#include "osdfile.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <iterator>
#include <limits>
#include <type_traits>


namespace util {

namespace {

// helper for holding a block of memory and deallocating it (or not) as necessary

template <typename T, bool Owned>
class ram_adapter_base : public virtual random_access
{
public:
	virtual ~ram_adapter_base()
	{
		if constexpr (Owned)
			std::free(m_data);
	}

	virtual std::error_condition seek(std::int64_t offset, int whence) noexcept override
	{
		switch (whence)
		{
		case SEEK_SET:
			if (0 > offset)
				return std::errc::invalid_argument;
			m_pointer = std::uint64_t(offset);
			return std::error_condition();

		case SEEK_CUR:
			if (0 > offset)
			{
				if (std::uint64_t(-offset) > m_pointer)
					return std::errc::invalid_argument;
			}
			else if ((std::numeric_limits<std::uint64_t>::max() - offset) < m_pointer)
			{
				return std::errc::invalid_argument;
			}
			m_pointer += offset;
			return std::error_condition();

		case SEEK_END:
			if (0 > offset)
			{
				if (std::uint64_t(-offset) > m_size)
					return std::errc::invalid_argument;
			}
			else if ((std::numeric_limits<std::uint64_t>::max() - offset) < m_size)
			{
				return std::errc::invalid_argument;
			}
			m_pointer = std::uint64_t(m_size) + offset;
			return std::error_condition();

		default:
			return std::errc::invalid_argument;
		}
	}

	virtual std::error_condition tell(std::uint64_t &result) noexcept override
	{
		result = m_pointer;
		return std::error_condition();
	}

	virtual std::error_condition length(std::uint64_t &result) noexcept override
	{
		if (std::numeric_limits<uint64_t>::max() < m_size)
			return std::errc::file_too_large;

		result = m_size;
		return std::error_condition();
	}

protected:
	template <typename U>
	ram_adapter_base(U *data, std::size_t size) noexcept : m_data(reinterpret_cast<T>(data)), m_size(size)
	{
		static_assert(sizeof(*m_data) == 1U, "Element type must be byte-sized");
		assert(m_data || !m_size);
	}

	T m_data;
	std::uint64_t m_pointer = 0U;
	std::size_t m_size;
};


// RAM read implementation

template <typename T, bool Owned>
class ram_read_adapter : public ram_adapter_base<T, Owned>, public virtual random_read
{
public:
	template <typename U>
	ram_read_adapter(U *data, std::size_t size) noexcept : ram_adapter_base<T, Owned>(data, size)
	{
	}

	virtual std::error_condition read(void *buffer, std::size_t length, std::size_t &actual) noexcept override
	{
		do_read(this->m_pointer, buffer, length, actual);
		this->m_pointer += actual;
		return std::error_condition();
	}

	virtual std::error_condition read_at(std::uint64_t offset, void *buffer, std::size_t length, std::size_t &actual) noexcept override
	{
		do_read(offset, buffer, length, actual);
		return std::error_condition();
	}

private:
	void do_read(std::uint64_t offset, void *buffer, std::size_t length, std::size_t &actual) const noexcept
	{
		if ((offset < this->m_size) && length)
		{
			actual = std::min(std::size_t(this->m_size - offset), length);
			if constexpr (Owned)
				std::memcpy(buffer, this->m_data + offset, actual);
			else
				std::memmove(buffer, this->m_data + offset, actual);
		}
		else
		{
			actual = 0U;
		}
	}
};


// helper class for holding an osd_file and closing it (or not) as necessary

class osd_file_adapter_base : public virtual random_access
{
public:
	virtual ~osd_file_adapter_base()
	{
		if (m_close)
			delete m_file;
	}

	virtual std::error_condition seek(std::int64_t offset, int whence) noexcept override
	{
		switch (whence)
		{
		case SEEK_SET:
			if (0 > offset)
				return std::errc::invalid_argument;
			m_pointer = std::uint64_t(offset);
			return std::error_condition();

		case SEEK_CUR:
			if (0 > offset)
			{
				if (std::uint64_t(-offset) > m_pointer)
					return std::errc::invalid_argument;
			}
			else if ((std::numeric_limits<std::uint64_t>::max() - offset) < m_pointer)
			{
				return std::errc::invalid_argument;
			}
			m_pointer += offset;
			return std::error_condition();

		// TODO: add SEEK_END when osd_file can support it - should it return a different error?
		default:
			return std::errc::invalid_argument;
		}
	}

	virtual std::error_condition tell(std::uint64_t &result) noexcept override
	{
		result = m_pointer;
		return std::error_condition();
	}

	virtual std::error_condition length(std::uint64_t &result) noexcept override
	{
		// not supported by osd_file
		return std::errc::not_supported; // TODO: revisit this error code
	}

protected:
	osd_file_adapter_base(osd_file::ptr &&file) noexcept : m_file(file.release()), m_close(true)
	{
		assert(m_file);
	}

	osd_file_adapter_base(osd_file &file) noexcept : m_file(&file), m_close(false)
	{
	}

	osd_file &file() noexcept
	{
		return *m_file;
	}

	std::uint64_t m_pointer = 0U;

private:
	osd_file *const m_file;
	bool const m_close;
};


// osd_file read implementation

class osd_file_read_adapter : public osd_file_adapter_base, public virtual random_read
{
public:
	osd_file_read_adapter(osd_file::ptr &&file) noexcept : osd_file_adapter_base(std::move(file))
	{
	}

	osd_file_read_adapter(osd_file &file) noexcept : osd_file_adapter_base(file)
	{
	}

	virtual std::error_condition read(void *buffer, std::size_t length, std::size_t &actual) noexcept override
	{
		// TODO: should the client have to deal with reading less than expected even if EOF isn't hit?
		if (std::numeric_limits<std::uint32_t>::max() < length)
		{
			actual = 0U;
			return std::errc::invalid_argument;
		}

		// actual length not valid on error
		std::uint32_t count;
		std::error_condition err = file().read(buffer, m_pointer, std::uint32_t(length), count);
		if (!err)
		{
			m_pointer += count;
			actual = std::size_t(count);
		}
		else
		{
			actual = 0U;
		}
		return err;
	}

	virtual std::error_condition read_at(std::uint64_t offset, void *buffer, std::size_t length, std::size_t &actual) noexcept override
	{
		// TODO: should the client have to deal with reading less than expected even if EOF isn't hit?
		if (std::numeric_limits<std::uint32_t>::max() < length)
		{
			actual = 0U;
			return std::errc::invalid_argument;
		}

		// actual length not valid on error
		std::uint32_t count;
		std::error_condition err = file().read(buffer, offset, std::uint32_t(length), count);
		if (!err)
			actual = std::size_t(count);
		else
			actual = 0U;
		return err;
	}
};


// osd_file read/write implementation

class osd_file_read_write_adapter : public osd_file_read_adapter, public random_read_write
{
public:
	using osd_file_read_adapter::osd_file_read_adapter;

	virtual std::error_condition finalize() noexcept override
	{
		return std::error_condition();
	}

	virtual std::error_condition flush() noexcept override
	{
		return file().flush();
	}

	virtual std::error_condition write(void const *buffer, std::size_t length, std::size_t &actual) noexcept override
	{
		actual = 0U;
		while (length)
		{
			// actual length not valid on error
			std::uint32_t const chunk = std::min<std::common_type_t<std::uint32_t, std::size_t> >(std::numeric_limits<std::uint32_t>::max(), length);
			std::uint32_t written;
			std::error_condition err = file().write(buffer, m_pointer, chunk, written);
			if (err)
				return err;
			m_pointer += written;
			buffer = reinterpret_cast<std::uint8_t const *>(buffer) + written;
			length -= written;
			actual += written;
		}
		return std::error_condition();
	}

	virtual std::error_condition write_at(std::uint64_t offset, void const *buffer, std::size_t length, std::size_t &actual) noexcept override
	{
		actual = 0U;
		while (length)
		{
			// actual length not valid on error
			std::uint32_t const chunk = std::min<std::common_type_t<std::uint32_t, std::size_t> >(std::numeric_limits<std::uint32_t>::max(), length);
			std::uint32_t written;
			std::error_condition err = file().write(buffer, offset, chunk, written);
			if (err)
				return err;
			offset += written;
			buffer = reinterpret_cast<std::uint8_t const *>(buffer) + written;
			length -= written;
			actual += written;
		}
		return std::error_condition();
	}
};

} // anonymous namespace


// creating RAM read adapters

random_read::ptr ram_read(void const *data, std::size_t size) noexcept
{
	random_read::ptr result;
	if (data || !size)
		result.reset(new (std::nothrow) ram_read_adapter<std::uint8_t const *const, false>(data, size));
	return result;
}

random_read::ptr ram_read(void const *data, std::size_t size, std::uint8_t filler) noexcept
{
	std::unique_ptr<random_read_fill_wrapper<ram_read_adapter<std::uint8_t const *const, false> > > result;
	if (data || !size)
		result.reset(new (std::nothrow) decltype(result)::element_type(data, size));
	if (result)
		result->set_filler(filler);
	return result;
}

random_read::ptr ram_read_copy(void const *data, std::size_t size) noexcept
{
	random_read::ptr result;
	void *const copy = size ? std::malloc(size) : nullptr;
	if (copy)
		std::memcpy(copy, data, size);
	if (copy || !size)
		result.reset(new (std::nothrow) ram_read_adapter<std::uint8_t *const, true>(copy, size));
	if (!result)
		std::free(copy);
	return result;
}

random_read::ptr ram_read_copy(void const *data, std::size_t size, std::uint8_t filler) noexcept
{
	std::unique_ptr<random_read_fill_wrapper<ram_read_adapter<std::uint8_t *const, true> > > result;
	void *const copy = size ? std::malloc(size) : nullptr;
	if (copy)
		std::memcpy(copy, data, size);
	if (copy || !size)
		result.reset(new (std::nothrow) decltype(result)::element_type(copy, size));
	if (!result)
		std::free(copy);
	return result;
}


// creating osd_file read adapters

random_read::ptr osd_file_read(osd_file::ptr &&file) noexcept
{
	random_read::ptr result;
	if (file)
		result.reset(new (std::nothrow) osd_file_read_adapter(std::move(file)));
	return result;
}

random_read::ptr osd_file_read(osd_file &file) noexcept
{
	return random_read::ptr(new (std::nothrow) osd_file_read_adapter(file));
}


// creating osd_file read/write adapters

random_read_write::ptr osd_file_read_write(osd_file::ptr &&file) noexcept
{
	random_read_write::ptr result;
	if (file)
		result.reset(new (std::nothrow) osd_file_read_write_adapter(std::move(file)));
	return result;
}

random_read_write::ptr osd_file_read_write(osd_file &file) noexcept
{
	return random_read_write::ptr(new (std::nothrow) osd_file_read_write_adapter(file));
}

} // namespace util
