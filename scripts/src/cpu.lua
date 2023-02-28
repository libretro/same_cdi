-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   cpu.lua
--
--   Rules for building CPU cores
--
---------------------------------------------------------------------------

--------------------------------------------------
-- Dynamic recompiler objects
--------------------------------------------------

DRC_CPUS = { "E1", "SH", "MIPS3", "POWERPC", "ARM7", "ADSP21062", "MB86235", "DSP16", "UNSP" }
CPU_INCLUDE_DRC = false
for i, v in ipairs(DRC_CPUS) do
	if (CPUS[v]~=null) then
		CPU_INCLUDE_DRC = true
		break
	end
end


if (CPU_INCLUDE_DRC) then
	files {
		MAME_DIR .. "src/devices/cpu/drcbec.cpp",
		MAME_DIR .. "src/devices/cpu/drcbec.h",
		MAME_DIR .. "src/devices/cpu/drcbeut.cpp",
		MAME_DIR .. "src/devices/cpu/drcbeut.h",
		MAME_DIR .. "src/devices/cpu/drccache.cpp",
		MAME_DIR .. "src/devices/cpu/drccache.h",
		MAME_DIR .. "src/devices/cpu/drcfe.cpp",
		MAME_DIR .. "src/devices/cpu/drcfe.h",
		MAME_DIR .. "src/devices/cpu/drcuml.cpp",
		MAME_DIR .. "src/devices/cpu/drcuml.h",
		MAME_DIR .. "src/devices/cpu/uml.cpp",
		MAME_DIR .. "src/devices/cpu/uml.h",
		MAME_DIR .. "src/devices/cpu/x86log.cpp",
		MAME_DIR .. "src/devices/cpu/x86log.h",
		MAME_DIR .. "src/devices/cpu/drcumlsh.h",
	}
	if not _OPTIONS["FORCE_DRC_C_BACKEND"] then
		files {
			MAME_DIR .. "src/devices/cpu/drcbex64.cpp",
			MAME_DIR .. "src/devices/cpu/drcbex64.h",
			MAME_DIR .. "src/devices/cpu/drcbex86.cpp",
			MAME_DIR .. "src/devices/cpu/drcbex86.h",
		}
	end
end

--------------------------------------------------
-- Motorola 6805
--@src/devices/cpu/m6805/m6805.h,CPUS["M6805"] = true
--------------------------------------------------

if CPUS["M6805"] then
	files {
		MAME_DIR .. "src/devices/cpu/m6805/m6805.cpp",
		MAME_DIR .. "src/devices/cpu/m6805/m6805.h",
		MAME_DIR .. "src/devices/cpu/m6805/m6805defs.h",
		MAME_DIR .. "src/devices/cpu/m6805/6805ops.hxx",
		MAME_DIR .. "src/devices/cpu/m6805/m68705.cpp",
		MAME_DIR .. "src/devices/cpu/m6805/m68705.h",
		MAME_DIR .. "src/devices/cpu/m6805/m68hc05.cpp",
		MAME_DIR .. "src/devices/cpu/m6805/m68hc05.h",
	}
end

if opt_tool(CPUS, "M6805") then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/m6805/6805dasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/m6805/6805dasm.h")
end

--------------------------------------------------
-- Intel 8051 and derivatives
--@src/devices/cpu/mcs51/mcs51.h,CPUS["MCS51"] = true
--------------------------------------------------

if CPUS["MCS51"] then
	files {
		MAME_DIR .. "src/devices/cpu/mcs51/mcs51.cpp",
		MAME_DIR .. "src/devices/cpu/mcs51/mcs51.h",
		MAME_DIR .. "src/devices/cpu/mcs51/mcs51ops.hxx",
		MAME_DIR .. "src/devices/cpu/mcs51/axc51-core.cpp",
		MAME_DIR .. "src/devices/cpu/mcs51/axc51-core.h",
	}
end

if opt_tool(CPUS, "MCS51") then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/mcs51/mcs51dasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/mcs51/mcs51dasm.h")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/mcs51/axc51-core_dasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/mcs51/axc51-core_dasm.h")
end

--------------------------------------------------
-- Motorola 68000 series
--@src/devices/cpu/m68000/m68000.h,CPUS["M680X0"] = true
--------------------------------------------------

if CPUS["M680X0"] then
	files {
		MAME_DIR .. "src/devices/cpu/m68000/m68kcpu.cpp",
		MAME_DIR .. "src/devices/cpu/m68000/m68kcpu.h",
		MAME_DIR .. "src/devices/cpu/m68000/m68kops.cpp",
		MAME_DIR .. "src/devices/cpu/m68000/m68kops.h",
		MAME_DIR .. "src/devices/cpu/m68000/m68000.h",
		MAME_DIR .. "src/devices/cpu/m68000/m68kfpu.cpp",
		MAME_DIR .. "src/devices/cpu/m68000/m68kmmu.h",
	}
end

if opt_tool(CPUS, "M680X0") then
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/m68000/m68kdasm.cpp")
	table.insert(disasm_files , MAME_DIR .. "src/devices/cpu/m68000/m68kdasm.h")
end
