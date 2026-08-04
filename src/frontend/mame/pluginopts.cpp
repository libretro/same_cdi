// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic
/***************************************************************************

    pluginopts.cpp

    Plugin options manager.

***************************************************************************/

#include "emu.h"
#include "pluginopts.h"
#include "options.h"


#include <fstream>


//**************************************************************************
//  PLUGIN OPTIONS
//**************************************************************************

//-------------------------------------------------
//  plugin_options - constructor
//-------------------------------------------------

plugin_options::plugin_options()
{
}


//-------------------------------------------------
//  scan_directory - plugin manifest discovery,
//  stubbed: the Lua plugin engine is removed from
//  this core, so plugin.json manifests (the only
//  rapidjson consumer in the tree) have nothing to
//  describe.  The class and its INI plumbing stay
//  so the option handling in mame.cpp and clifront
//  is untouched.
//-------------------------------------------------

void plugin_options::scan_directory(const std::string &path, bool recursive)
{
}


//-------------------------------------------------
//  load_plugin - stubbed with scan_directory
//-------------------------------------------------

bool plugin_options::load_plugin(const std::string &path)
{
	return false;
}


plugin_options::plugin *plugin_options::find(const std::string &name)
{
	auto iter = std::find_if(
		m_plugins.begin(),
		m_plugins.end(),
		[&name](const plugin &p) { return name == p.m_name; });

	return iter != m_plugins.end()
		? &*iter
		: nullptr;
}


//-------------------------------------------------
//  create_core_options
//-------------------------------------------------

static core_options create_core_options(const plugin_options &plugin_opts)
{
	// we're sort of abusing core_options to just get INI file parsing, so we'll build a
	// core_options structure for the sole purpose of parsing an INI file, and then reflect
	// the data back
	static const options_entry s_option_entries[] =
	{
		{ nullptr, nullptr, OPTION_HEADER, "PLUGINS OPTIONS" },
		{ nullptr }
	};

	core_options opts;
	opts.add_entries(s_option_entries);

	// create an entry for each option
	for (const plugin_options::plugin &p : plugin_opts.plugins())
	{
		opts.add_entry(
			{ p.m_name },
			nullptr,
			core_options::option_type::BOOLEAN,
			p.m_start ? "1" : "0");
	}

	return opts;
}


//-------------------------------------------------
//  parse_ini_file
//-------------------------------------------------

void plugin_options::parse_ini_file(util::core_file &inifile)
{
	core_options opts = create_core_options(*this);

	// parse the INI file
	opts.parse_ini_file(inifile, OPTION_PRIORITY_NORMAL, true, true);

	// and reflect these options back
	for (plugin &p : m_plugins)
		p.m_start = opts.bool_value(p.m_name);
}


//-------------------------------------------------
//  output_ini
//-------------------------------------------------

std::string plugin_options::output_ini() const
{
	core_options opts = create_core_options(*this);
	return opts.output_ini();
}
