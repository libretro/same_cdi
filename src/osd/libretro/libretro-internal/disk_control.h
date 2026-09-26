// license:BSD-3-Clause
// copyright-holders:vibecodekun
// Libretro disc list and tray control. Included by libretro.cpp.
#ifndef SAME_CDI_DISK_CONTROL_H
#define SAME_CDI_DISK_CONTROL_H

#include "imagedev/chd_cd.h"
#include "corefile.h"
#include "corestr.h"
#include "machine/cdicdic.h"
#include <limits>
#include <string>
#include <vector>

namespace disk_control {

static std::vector<std::string> images;
static unsigned index = 0;
static bool ejected = false;
static bool loaded = false;
static unsigned initial_index = 0;
static std::string initial_path;

static void clear()
{
   images.clear();
   index = 0;
   ejected = loaded = false;
   initial_index = 0;
   initial_path.clear();
}

static bool is_disc(const std::string &path)
{
   const auto dot = path.find_last_of('.');
   if (dot == std::string::npos)
      return false;
   const char *ext = path.c_str() + dot;
   return !core_stricmp(ext, ".cue") || !core_stricmp(ext, ".chd") || !core_stricmp(ext, ".iso");
}

static cdrom_image_device *drive()
{
   auto *manager = mame_machine_manager::instance();
   return loaded && manager && manager->machine()
      ? manager->machine()->root_device().subdevice<cdrom_image_device>("cdrom") : nullptr;
}

// Frontend state extension: the medium itself stays selected by the frontend.
// Record its identity so restoring RAM from another disc can notify the guest
// without unloading content or disturbing same-medium runahead restores.
static constexpr size_t state_size = 16;

static uint64_t media_identity()
{
   auto *cdrom = drive();
   if (!cdrom || !cdrom->get_cdrom_file())
      return 0;
   uint64_t hash = UINT64_C(14695981039346656037);
   const char *path = cdrom->filename();
   if (path)
      for (; *path; ++path)
      {
         const uint8_t c = *path == '\\' ? '/' : uint8_t(*path);
         hash = (hash ^ c) * UINT64_C(1099511628211);
      }
   return hash ? hash : 1;
}

static void write_state(uint8_t *data)
{
   memcpy(data, "CDIMEDIA", 8);
   const uint64_t id = media_identity();
   for (unsigned i = 0; i < 8; ++i)
      data[8 + i] = uint8_t(id >> (i * 8));
}

static bool valid_state(const uint8_t *data, size_t size)
{
   if (size >= state_size && !memcmp(data, "CDIMEDIA", 8))
      return true;
   // Older serializers could zero-pad a larger frontend buffer. Preserve
   // that compatibility, but reject a truncated or damaged media extension.
   for (size_t i = 0; i < size; ++i)
      if (data[i])
         return false;
   return true;
}

static void restored(const uint8_t *data, size_t size)
{
   auto *cdrom = drive();
   if (!cdrom)
      return;
   bool mismatch = false;
   if (size >= state_size && !memcmp(data, "CDIMEDIA", 8))
   {
      uint64_t saved = 0;
      for (unsigned i = 0; i < 8; ++i)
         saved |= uint64_t(data[8 + i]) << (i * 8);
      mismatch = saved != media_identity();
   }
   else if (auto *cdic = cdrom->machine().root_device().subdevice<cdicdic_device>("cdic"))
      mismatch = cdic->legacy_media_mismatch();
   if (mismatch)
      cdrom->notify_media_change();
}

static bool set_eject_state(bool state)
{
   auto *cdrom = drive();
   if (!cdrom)
      return false;
   if (state == ejected)
      return true;
   if (state)
      cdrom->unload();
   else if (index < images.size())
   {
      if (images[index].empty() || cdrom->load(images[index]) != image_init_result::PASS)
      {
         if (log_cb)
            log_cb(RETRO_LOG_ERROR, "Unable to insert disc: %s\n", images[index].c_str());
         return false; // Keep the tray open so the frontend can retry.
      }
   }
   ejected = state;
   return true;
}

static bool get_eject_state() { return ejected; }
static unsigned get_image_index() { return index; }
static unsigned get_num_images() { return unsigned(images.size()); }

static bool set_image_index(unsigned value)
{
   if (!loaded || !ejected)
      return false;
   // Every out-of-range index means an empty tray in the libretro API.
   index = value < images.size() ? value : unsigned(images.size());
   return true;
}

static bool replace_image_index(unsigned value, const retro_game_info *info)
{
   if (!loaded || !ejected || value >= images.size())
      return false;
   if (info)
   {
      if (!info->path || !is_disc(info->path))
         return false;
      images[value] = info->path;
   }
   else
   {
      const bool removed_selected = index == value;
      images.erase(images.begin() + value);
      if (removed_selected)
         index = unsigned(images.size());
      else if (index > value)
         --index;
   }
   return true;
}

static bool add_image_index()
{
   if (!loaded || images.size() == std::numeric_limits<unsigned>::max())
      return false;
   const bool no_disc = index == images.size();
   images.emplace_back();
   if (no_disc)
      index = unsigned(images.size());
   return true;
}

static bool set_initial_image(unsigned value, const char *path)
{
   if (loaded || !path || !*path)
      return false;
   initial_index = value;
   initial_path = path;
   return true;
}

static bool copy_string(const std::string &value, char *out, size_t size)
{
   if (!out || !size)
      return false;
   out[0] = '\0';
   if (value.empty() || value.size() >= size)
      return false;
   memcpy(out, value.c_str(), value.size() + 1);
   return true;
}

static bool get_image_path(unsigned value, char *out, size_t size)
{
   return copy_string(value < images.size() ? images[value] : std::string(), out, size);
}

static bool get_image_label(unsigned value, char *out, size_t size)
{
   if (value >= images.size())
      return copy_string({}, out, size);
   std::string label = images[value];
   const auto slash = label.find_last_of("/\\");
   if (slash != std::string::npos)
      label.erase(0, slash + 1);
   const auto dot = label.find_last_of('.');
   if (dot != std::string::npos)
      label.erase(dot);
   return copy_string(label, out, size);
}

static void register_interface()
{
   static retro_disk_control_ext_callback extended = {
      set_eject_state, get_eject_state, get_image_index, set_image_index,
      get_num_images, replace_image_index, add_image_index,
      set_initial_image, get_image_path, get_image_label
   };
   static retro_disk_control_callback basic = {
      set_eject_state, get_eject_state, get_image_index, set_image_index,
      get_num_images, replace_image_index, add_image_index
   };
   unsigned version = 0;
   if (environ_cb(RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION, &version) && version >= 1 &&
       environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, &extended))
      return;
   environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &basic);
}

// Resolve playlist entries relative to the playlist, not the process directory.
static bool prepare(const char *path)
{
   images.clear();
   index = 0;
   ejected = loaded = false;
   if (!path || !*path)
      return false;
   const std::string content(path);
   const auto dot = content.find_last_of('.');
   if (dot != std::string::npos && !core_stricmp(content.c_str() + dot, ".m3u"))
   {
      util::core_file::ptr file;
      if (util::core_file::open(content, OPEN_FLAG_READ, file))
         return false;
      const auto slash = content.find_last_of("/\\");
      const std::string directory = slash == std::string::npos ? "" : content.substr(0, slash + 1);
      // core_file::gets does not terminate a full buffer and normalises EOL to CR.
      char buffer[4096] = {};
      while (file->gets(buffer, sizeof(buffer) - 1))
      {
         std::string line(buffer);
         if (line.size() == sizeof(buffer) - 1 && line.back() != '\r')
            return false;
         if (line.compare(0, 3, "\xef\xbb\xbf") == 0)
            line.erase(0, 3);
         const auto first = line.find_first_not_of(" \t\r\n");
         if (first == std::string::npos || line[first] == '#')
            continue;
         line = line.substr(first, line.find_last_not_of(" \t\r\n") - first + 1);
         if (!is_disc(line))
            return false;
         if (line[0] != '/' && line[0] != '\\' && !(line.size() > 1 && line[1] == ':'))
            line = directory + line;
         images.push_back(std::move(line));
      }
   }
   else if (is_disc(content))
      images.push_back(content);
   if (images.empty())
      return false;
   if (initial_index < images.size() && images[initial_index] == initial_path)
      index = initial_index;
   initial_index = 0;
   initial_path.clear();
   return true;
}

} // namespace disk_control
#endif
