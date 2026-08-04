// license:BSD-3-Clause
// copyright-holders:Nicola Salmoria, Aaron Giles
/****************************************************************************

    debugger.cpp

    Stub debugger front-end.  The libretro OSD only registers the "none"
    debugger module, so the interactive debugger (commands, console,
    expression help, views, breakpoints/watchpoints, disassembly and
    trace buffers) is unreachable in this core and has been removed.
    This manager keeps the API surface the rest of the emulator links
    against; everything is inert.

****************************************************************************/

#include "emu.h"
#include "debugger.h"
#include "debug/debugcon.h"
#include "debug/debugcpu.h"

debugger_manager::debugger_manager(running_machine &machine)
	: m_machine(machine)
{
	m_cpu = std::make_unique<debugger_cpu>(machine);
	m_console = std::make_unique<debugger_console>(machine);
}

debugger_manager::~debugger_manager()
{
}

void debugger_manager::debug_break()
{
}

bool debugger_manager::within_instruction_hook()
{
	return false;
}

void debugger_manager::refresh_display()
{
}

void debugger_flush_all_traces_on_abnormal_exit()
{
}
