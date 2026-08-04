// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*********************************************************************

    debugcpu.h

    Stub debugger CPU/device interfaces (see debugger.cpp for the
    rationale).  Keeps the classes the emulator core links against;
    every hook is inert.

*********************************************************************/

#ifndef MAME_EMU_DEBUG_DEBUGCPU_H
#define MAME_EMU_DEBUG_DEBUGCPU_H

#pragma once


// ======================> device_debug

class device_debug
{
public:
	device_debug(device_t &device) { }
	~device_debug() { }

	// hooks used by the scheduler and execution cores
	void start_hook(const attotime &endtime) { }
	void stop_hook() { }
	void interrupt_hook(int irqline) { }
	void exception_hook(int exception) { }
	void privilege_hook() { }
	void instruction_hook(offs_t curpc) { }

	// execution control (osd "none" debugger module)
	void go(offs_t targetpc = ~0) { }
};


// ======================> debugger_cpu

class debugger_cpu
{
public:
	debugger_cpu(running_machine &machine) { }

	void comment_save() { }
};

#endif // MAME_EMU_DEBUG_DEBUGCPU_H
