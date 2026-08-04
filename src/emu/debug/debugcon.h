// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*********************************************************************

    debugcon.h

    Stub debugger console (see debugger.cpp for rationale).

*********************************************************************/

#ifndef MAME_EMU_DEBUG_DEBUGCON_H
#define MAME_EMU_DEBUG_DEBUGCON_H

#pragma once

class debugger_console
{
public:
	debugger_console(running_machine &machine) : m_machine(machine) { }

	device_t *get_visible_cpu() { return nullptr; }
	void set_visible_cpu(device_t *device) { }

	template <typename Format, typename... Params>
	void printf(Format &&fmt, Params &&...args) { }

private:
	running_machine &m_machine;
};

#endif // MAME_EMU_DEBUG_DEBUGCON_H
