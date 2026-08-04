// license:BSD-3-Clause
// copyright-holders:Olivier Galibert, R. Belmont, Vas Crabb
//============================================================
//
//  vfsfile.cpp - osd_file and osd::directory on libretro VFS
//
//  Every file and directory access in the core funnels through
//  osd_file / osd::directory; this implementation routes all of it
//  through libretro-common's file streams, which use the frontend's
//  VFS interface when RETRO_ENVIRONMENT_GET_VFS_INTERFACE provides
//  one and libretro-common's local implementation otherwise.  It
//  replaces the posix*/win* platform modules for every platform.
//
//  Deliberate deltas from the platform modules, none exercised by
//  the CD-i machine set:
//  - "socket.", "domain." and PTY pseudo-paths are refused with
//    not_supported: no compiled device uses the bitbanger/MIDI
//    comms paths that need them.
//  - directory entries and osd_stat report the epoch for
//    last_modified: the VFS stat interface carries no mtime.  The
//    only consumer is the save-state menu's date sort.
//
//============================================================

#include "osdcore.h"
#include "osdfile.h"

#include "unicode.h"

#include <streams/file_stream.h>
#include <file/file_path.h>
#include <retro_dirent.h>

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <direct.h> // _getcwd
constexpr char PATHSEPCH = '\\';
constexpr char INVPATHSEPCH = '/';
#else
#include <climits>
#include <cstdio>
#include <unistd.h> // getcwd
constexpr char PATHSEPCH = '/';
#endif

namespace {

//============================================================
//  pseudo-path detection (refused: no compiled consumer)
//============================================================

bool is_pseudo_path(std::string const &path) noexcept
{
	return !path.compare(0, strlen("socket."), "socket.")
		|| !path.compare(0, strlen("domain."), "domain.")
		|| !path.compare(0, strlen("/dev/ptmx"), "/dev/ptmx");
}


//============================================================
//  vfs_osd_file
//============================================================

class vfs_osd_file : public osd_file
{
public:
	vfs_osd_file(vfs_osd_file const &) = delete;
	vfs_osd_file(vfs_osd_file &&) = delete;
	vfs_osd_file &operator=(vfs_osd_file const &) = delete;
	vfs_osd_file &operator=(vfs_osd_file &&) = delete;

	vfs_osd_file(RFILE *file) noexcept : m_file(file)
	{
		assert(m_file);
	}

	virtual ~vfs_osd_file()
	{
		filestream_close(m_file);
	}

	virtual std::error_condition read(void *buffer, std::uint64_t offset, std::uint32_t count, std::uint32_t &actual) noexcept override
	{
		if (filestream_seek(m_file, int64_t(offset), RETRO_VFS_SEEK_POSITION_START) < 0)
			return std::errc::invalid_argument;
		int64_t const result = filestream_read(m_file, buffer, int64_t(count));
		if (result < 0)
			return std::errc::io_error;
		actual = std::uint32_t(result);
		return std::error_condition();
	}

	virtual std::error_condition write(void const *buffer, std::uint64_t offset, std::uint32_t count, std::uint32_t &actual) noexcept override
	{
		if (filestream_seek(m_file, int64_t(offset), RETRO_VFS_SEEK_POSITION_START) < 0)
			return std::errc::invalid_argument;
		int64_t const result = filestream_write(m_file, buffer, int64_t(count));
		if (result < 0)
			return std::errc::io_error;
		actual = std::uint32_t(result);
		return std::error_condition();
	}

	virtual std::error_condition truncate(std::uint64_t offset) noexcept override
	{
		if (filestream_truncate(m_file, int64_t(offset)) < 0)
			return std::errc::io_error;
		return std::error_condition();
	}

	virtual std::error_condition flush() noexcept override
	{
		if (filestream_flush(m_file) < 0)
			return std::errc::io_error;
		return std::error_condition();
	}

private:
	RFILE *m_file;
};


RFILE *open_rfile(std::string const &path, std::uint32_t openflags) noexcept
{
	unsigned mode;
	if (openflags & OPEN_FLAG_WRITE)
	{
		if (openflags & OPEN_FLAG_CREATE)
		{
			// created and truncated, matching O_CREAT | O_TRUNC
			mode = (openflags & OPEN_FLAG_READ)
					? (RETRO_VFS_FILE_ACCESS_READ_WRITE)
					: (RETRO_VFS_FILE_ACCESS_WRITE);
		}
		else
		{
			// existing file, kept: "r+b"
			mode = RETRO_VFS_FILE_ACCESS_READ_WRITE | RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;
		}
	}
	else if (openflags & OPEN_FLAG_READ)
	{
		mode = RETRO_VFS_FILE_ACCESS_READ;
	}
	else
	{
		return nullptr;
	}

	return filestream_open(path.c_str(), mode, RETRO_VFS_FILE_ACCESS_HINT_NONE);
}

} // anonymous namespace


//============================================================
//  osd_file::open
//============================================================

std::error_condition osd_file::open(std::string const &path, std::uint32_t openflags, ptr &file, std::uint64_t &filesize) noexcept
{
	if (is_pseudo_path(path))
		return std::errc::not_supported;

	if (!(openflags & (OPEN_FLAG_READ | OPEN_FLAG_WRITE)))
		return std::errc::invalid_argument;

	std::string dst;
	try
	{
		dst = path;
#if defined(_WIN32)
		for (auto it = dst.begin(); it != dst.end(); ++it)
			*it = (INVPATHSEPCH == *it) ? PATHSEPCH : *it;
#endif
		osd_subst_env(dst, dst);
	}
	catch (...)
	{
		return std::errc::not_enough_memory;
	}

	RFILE *f = open_rfile(dst, openflags);
	if (!f && (openflags & OPEN_FLAG_WRITE) && (openflags & OPEN_FLAG_CREATE) && (openflags & OPEN_FLAG_CREATE_PATHS))
	{
		// create the missing directory chain and retry, matching the
		// platform modules' behaviour for OPEN_FLAG_CREATE_PATHS
		auto const sep = dst.rfind(PATHSEPCH);
		if ((sep != std::string::npos) && sep)
		{
			try
			{
				std::string const parent(dst.substr(0, sep));
				if (path_mkdir(parent.c_str()))
					f = open_rfile(dst, openflags);
			}
			catch (...)
			{
				return std::errc::not_enough_memory;
			}
		}
	}
	if (!f)
	{
		// libretro streams do not surface an error code; a missing
		// file is by far the common case (search paths are probed)
		return std::errc::no_such_file_or_directory;
	}

	int64_t const size = filestream_get_size(f);

	osd_file::ptr result(new (std::nothrow) vfs_osd_file(f));
	if (!result)
	{
		filestream_close(f);
		return std::errc::not_enough_memory;
	}
	file = std::move(result);
	filesize = (size < 0) ? 0 : std::uint64_t(size);
	return std::error_condition();
}


//============================================================
//  osd_file::openpty
//============================================================

std::error_condition osd_file::openpty(ptr &file, std::string &name) noexcept
{
	return std::errc::not_supported;
}


//============================================================
//  osd_file::remove
//============================================================

std::error_condition osd_file::remove(std::string const &filename) noexcept
{
	if (filestream_delete(filename.c_str()) != 0)
		return std::errc::no_such_file_or_directory;
	return std::error_condition();
}


//============================================================
//  osd_get_physical_drive_geometry
//============================================================

bool osd_get_physical_drive_geometry(const char *filename, uint32_t *cylinders, uint32_t *heads, uint32_t *sectors, uint32_t *bps) noexcept
{
	return false; // no, no way, huh-uh, forget it
}


//============================================================
//  osd_stat
//============================================================

std::unique_ptr<osd::directory::entry> osd_stat(const std::string &path)
{
	int const flags = path_stat(path.c_str());
	if (!(flags & RETRO_VFS_STAT_IS_VALID))
		return nullptr;
	int64_t size = path_get_size(path.c_str());
	if (size < 0)
		size = 0;

	// one allocation the caller frees as a unit, name storage appended
	osd::directory::entry *result;
	try { result = reinterpret_cast<osd::directory::entry *>(::operator new(sizeof(*result) + path.length() + 1)); }
	catch (...) { return nullptr; }
	new (result) osd::directory::entry;

	std::strcpy(reinterpret_cast<char *>(result) + sizeof(*result), path.c_str());
	result->name = reinterpret_cast<char *>(result) + sizeof(*result);
	result->type = (flags & RETRO_VFS_STAT_IS_DIRECTORY)
			? osd::directory::entry::entry_type::DIR
			: osd::directory::entry::entry_type::FILE;
	result->size = std::uint64_t(size);
	result->last_modified = std::chrono::system_clock::time_point(); // VFS stat has no mtime

	return std::unique_ptr<osd::directory::entry>(result);
}


//============================================================
//  osd_get_full_path
//============================================================

std::error_condition osd_get_full_path(std::string &dst, std::string const &path) noexcept
{
	// Purely lexical resolution: an absolute path is normalised in
	// place, a relative one is joined to the current directory first.
	// The platform canonicalisers (realpath/_fullpath) walked the
	// filesystem to resolve symlinks, which is file I/O outside the
	// VFS; MAME only needs a stable absolute form for path joins and
	// display, which this provides without touching the filesystem.
	try
	{
		std::string joined;
		if (path_is_absolute(path.c_str()))
		{
			joined = path;
		}
		else
		{
#if defined(_WIN32)
			std::vector<char> cwd(260);
			if (!::_getcwd(&cwd[0], int(cwd.size())))
				return std::error_condition(errno, std::generic_category());
#else
			std::vector<char> cwd(PATH_MAX);
			if (!::getcwd(&cwd[0], cwd.size()))
				return std::error_condition(errno, std::generic_category());
#endif
			joined = &cwd[0];
			joined += PATHSEPCH;
			joined += path;
		}
#if defined(_WIN32)
		for (auto &ch : joined)
			if (ch == INVPATHSEPCH)
				ch = PATHSEPCH;
#endif
		// collapse repeated separators and "." / ".." components
		std::vector<std::string> parts;
		std::string prefix;
		std::size_t pos = 0;
#if defined(_WIN32)
		if ((joined.length() >= 2) && (joined[1] == ':'))
		{
			prefix = joined.substr(0, 2);
			pos = 2;
		}
#endif
		while (pos < joined.length())
		{
			std::size_t const next = joined.find(PATHSEPCH, pos);
			std::string const comp = joined.substr(pos, (next == std::string::npos) ? std::string::npos : (next - pos));
			pos = (next == std::string::npos) ? joined.length() : (next + 1);
			if (comp.empty() || (comp == "."))
				continue;
			else if (comp == "..")
			{
				if (!parts.empty())
					parts.pop_back();
			}
			else
				parts.push_back(comp);
		}
		dst = prefix;
		for (auto const &comp : parts)
		{
			dst += PATHSEPCH;
			dst += comp;
		}
		if (dst.empty() || (dst == prefix))
			dst = prefix + PATHSEPCH;
		return std::error_condition();
	}
	catch (...)
	{
		return std::errc::not_enough_memory;
	}
}


//============================================================
//  osd_subst_env - $VAR substitution, carried over from the
//  platform modules (environment access, not file I/O)
//============================================================

void osd_subst_env(std::string &dst, std::string const &src)
{
	std::string result, var;
	auto start = src.begin();

	// a leading tilde expands as $HOME
	if ((src.end() != start) && ('~' == *start))
	{
		char const *const home = std::getenv("HOME");
		if (home)
		{
			++start;
			if ((src.end() == start) || (PATHSEPCH == *start))
				result.append(home);
			else
				result.push_back('~');
		}
	}

	while (src.end() != start)
	{
		// find $ marking start of environment variable or end of string
		auto it = start;
		while ((src.end() != it) && ('$' != *it)) ++it;
		if (start != it) result.append(start, it);
		start = it;

		if (src.end() != start)
		{
			start = ++it;
			if ((src.end() != start) && ('{' == *start))
			{
				start = ++it;
				for (++it; (src.end() != it) && ('}' != *it); ++it) { }
				if (src.end() == it)
				{
					result.append("${").append(start, it);
					start = it;
				}
				else
				{
					var.assign(start, it);
					start = ++it;
					const char *const exp = std::getenv(var.c_str());
					if (exp)
						result.append(exp);
					else
						fprintf(stderr, "Warning: osd_subst_env variable %s not found.\n", var.c_str());
				}
			}
			else if ((src.end() != start) && (('_' == *start) || std::isalnum(*start)))
			{
				for (++it; (src.end() != it) && (('_' == *it) || std::isalnum(*it)); ++it) { }
				var.assign(start, it);
				start = it;
				const char *const exp = std::getenv(var.c_str());
				if (exp)
					result.append(exp);
				else
					fprintf(stderr, "Warning: osd_subst_env variable %s not found.\n", var.c_str());
			}
			else
			{
				result.push_back('$');
			}
		}
	}

	dst = std::move(result);
}


//============================================================
//  osd_is_absolute_path
//============================================================

bool osd_is_absolute_path(std::string const &path) noexcept
{
	return path_is_absolute(path.c_str());
}


//============================================================
//  osd_get_volume_names - the UI's root-of-filesystem list;
//  a single "/" as on the posix module (Windows drive letters
//  were only ever enumerated by the win32 module)
//============================================================

std::vector<std::string> osd_get_volume_names()
{
	return std::vector<std::string>{ std::string("/") };
}


//============================================================
//  osd_is_valid_filename_char / osd_is_valid_filepath_char
//============================================================

bool osd_is_valid_filename_char(char32_t uchar) noexcept
{
	return osd_is_valid_filepath_char(uchar)
#if defined(_WIN32)
		&& uchar != PATHSEPCH
		&& uchar != INVPATHSEPCH
#else
		&& uchar != PATHSEPCH
		&& uchar != '\\'
#endif
		&& uchar != ':';
}

bool osd_is_valid_filepath_char(char32_t uchar) noexcept
{
	return uchar >= 0x20
		&& !(uchar >= '\x7F' && uchar <= '\x9F')
#if defined(_WIN32)
		&& uchar != '<'
		&& uchar != '>'
		&& uchar != '\"'
		&& uchar != '|'
		&& uchar != '?'
		&& uchar != '*'
#endif
		&& uchar_isvalid(uchar);
}


//============================================================
//  osd::directory on retro_dirent
//============================================================

namespace osd {

namespace {

class vfs_directory : public directory
{
public:
	vfs_directory(RDIR *dir, std::string &&path) noexcept
		: m_dir(dir)
		, m_path(std::move(path))
	{
	}

	virtual ~vfs_directory() override
	{
		retro_closedir(m_dir);
	}

	virtual const entry *read() override
	{
		if (!retro_readdir(m_dir))
			return nullptr;

		char const *const name = retro_dirent_get_name(m_dir);
		if (!name)
			return nullptr;

		try
		{
			m_name = name;
			m_entry.name = m_name.c_str();
			if (retro_dirent_is_dir(m_dir, nullptr))
			{
				m_entry.type = entry::entry_type::DIR;
				m_entry.size = 0;
			}
			else
			{
				m_entry.type = entry::entry_type::FILE;
				std::string full(m_path);
				if (!full.empty() && (full.back() != PATHSEPCH))
					full += PATHSEPCH;
				full += m_name;
				int64_t const size = path_get_size(full.c_str());
				m_entry.size = (size < 0) ? 0 : std::uint64_t(size);
			}
			m_entry.last_modified = std::chrono::system_clock::time_point(); // VFS stat has no mtime
			return &m_entry;
		}
		catch (...)
		{
			return nullptr;
		}
	}

private:
	RDIR *      m_dir;
	std::string m_path;
	std::string m_name;
	entry       m_entry;
};

} // anonymous namespace

directory::ptr directory::open(std::string const &dirname)
{
	RDIR *const dir = retro_opendir(dirname.c_str());
	if (!dir)
		return nullptr;

	try
	{
		return ptr(new vfs_directory(dir, std::string(dirname)));
	}
	catch (...)
	{
		retro_closedir(dir);
		return nullptr;
	}
}

} // namespace osd
