// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  winutil.cpp - Win32 OSD core utility functions
//
//============================================================

// standard windows headers
#include <windows.h>
#include <direct.h>

// MAME headers
#include "emu.h"

// MAMEOS headers
#include "winutil.h"
#include "strconv.h"
#include "timeconv.h"


//============================================================
//  win_attributes_to_entry_type
//============================================================

osd::directory::entry::entry_type win_attributes_to_entry_type(DWORD attributes)
{
	if (attributes == 0xFFFFFFFF)
		return osd::directory::entry::entry_type::NONE;
	else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		return osd::directory::entry::entry_type::DIR;
	return osd::directory::entry::entry_type::FILE;
}



//============================================================
//  win_time_point_from_filetime
//============================================================

std::chrono::system_clock::time_point win_time_point_from_filetime(LPFILETIME file_time)
{
	auto converted_file_time = util::ntfs_duration_from_filetime(file_time->dwHighDateTime, file_time->dwLowDateTime);
	return util::system_clock_time_point_from_ntfs_duration(converted_file_time);
}

//============================================================
//  osd_subst_env
//============================================================
void osd_subst_env(std::string &dst, const std::string &src)
{
	std::wstring const w_src = osd::text::to_wstring(src);
	std::vector<wchar_t> buffer(w_src.size() + 2);
	DWORD length(ExpandEnvironmentStringsW(w_src.c_str(), &buffer[0], buffer.size()));
	while (length && (buffer.size() < length))
	{
		buffer.clear();
		buffer.resize(length + 1);
		length = ExpandEnvironmentStringsW(w_src.c_str(), &buffer[0], buffer.size());
	}
	if (length)
		osd::text::from_wstring(dst, &buffer[0]);
	else
		dst.clear();
}
