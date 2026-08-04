// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic,Luca Bruno
/***************************************************************************

    luaengine.h

    Stub replacement for the Lua scripting engine.  Lua, sol2 and the
    lualibs modules (lsqlite3, lua-zlib, lfs, lua-linenoise) have been
    removed from this core; no plugins are shipped and no console is
    exposed, so every entry point below is inert.  The public surface
    matches the subset of the original lua_engine used by the frontend
    so call sites compile unchanged.

***************************************************************************/
#ifndef MAME_FRONTEND_MAME_LUAENGINE_H
#define MAME_FRONTEND_MAME_LUAENGINE_H

#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

class running_machine;

class lua_engine
{
public:
	lua_engine() { }
	~lua_engine() { }

	void initialize() { }
	void load_script(const char *filename) { }
	void load_string(const char *value) { }

	bool frame_hook() { return false; }

	std::optional<long> menu_populate(const std::string &menu, std::vector<std::tuple<std::string, std::string, std::string> > &menu_list, std::string &flags) { return std::nullopt; }
	std::pair<bool, std::optional<long> > menu_callback(const std::string &menu, int index, const std::string &event) { return std::make_pair(false, std::nullopt); }

	void set_machine(running_machine *machine) { }
	std::vector<std::string> &get_menu() { return m_menu; }
	void attach_notifiers() { }
	void on_frame_done() { }
	void on_sound_update() { }
	void on_periodic() { }
	bool on_missing_mandatory_image(const std::string &instance_name) { return false; }
	void on_machine_before_load_settings() { }

	template <typename T, typename U>
	bool call_plugin(const std::string &name, T &&in, U &out) { return false; }

	template <typename T, typename U>
	bool call_plugin(const std::string &name, T &&in, std::vector<U> &out) { return false; }

	template <typename T, typename U = T>
	bool call_plugin_check(const std::string &name, U &&in, bool table = false) { return false; }

	template <typename T>
	void call_plugin_set(const std::string &name, T &&in) { }

private:
	std::vector<std::string> m_menu;
};

#endif // MAME_FRONTEND_MAME_LUAENGINE_H
