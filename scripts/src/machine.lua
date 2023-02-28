-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   machine.lua
--
--   Rules for building machine cores
--
----------------------------------------------------------------------------

files {
	MAME_DIR .. "src/devices/machine/bcreader.cpp",
	MAME_DIR .. "src/devices/machine/bcreader.h",
	MAME_DIR .. "src/devices/machine/buffer.cpp",
	MAME_DIR .. "src/devices/machine/buffer.h",
	MAME_DIR .. "src/devices/machine/clock.cpp",
	MAME_DIR .. "src/devices/machine/clock.h",
	MAME_DIR .. "src/devices/machine/keyboard.cpp",
	MAME_DIR .. "src/devices/machine/keyboard.h",
	MAME_DIR .. "src/devices/machine/keyboard.ipp",
	MAME_DIR .. "src/devices/machine/laserdsc.cpp",
	MAME_DIR .. "src/devices/machine/laserdsc.h",
	MAME_DIR .. "src/devices/machine/legscsi.cpp",
	MAME_DIR .. "src/devices/machine/legscsi.h",
	MAME_DIR .. "src/devices/machine/nvram.cpp",
	MAME_DIR .. "src/devices/machine/nvram.h",
	MAME_DIR .. "src/devices/machine/ram.cpp",
	MAME_DIR .. "src/devices/machine/ram.h",
	MAME_DIR .. "src/devices/machine/rescap.h",
	MAME_DIR .. "src/devices/machine/sdlc.cpp",
	MAME_DIR .. "src/devices/machine/sdlc.h",
	MAME_DIR .. "src/devices/machine/terminal.cpp",
	MAME_DIR .. "src/devices/machine/terminal.h",
	MAME_DIR .. "src/devices/machine/timer.cpp",
	MAME_DIR .. "src/devices/machine/timer.h",
}
files {
	MAME_DIR .. "src/devices/imagedev/bitbngr.cpp",
	MAME_DIR .. "src/devices/imagedev/bitbngr.h",
	MAME_DIR .. "src/devices/imagedev/cassette.cpp",
	MAME_DIR .. "src/devices/imagedev/cassette.h",
	MAME_DIR .. "src/devices/imagedev/chd_cd.cpp",
	MAME_DIR .. "src/devices/imagedev/chd_cd.h",
	MAME_DIR .. "src/devices/imagedev/diablo.cpp",
	MAME_DIR .. "src/devices/imagedev/diablo.h",
	MAME_DIR .. "src/devices/imagedev/flopdrv.cpp",
	MAME_DIR .. "src/devices/imagedev/flopdrv.h",
	MAME_DIR .. "src/devices/imagedev/floppy.cpp",
	MAME_DIR .. "src/devices/imagedev/floppy.h",
	MAME_DIR .. "src/devices/imagedev/harddriv.cpp",
	MAME_DIR .. "src/devices/imagedev/harddriv.h",
	MAME_DIR .. "src/devices/imagedev/mfmhd.cpp",
	MAME_DIR .. "src/devices/imagedev/mfmhd.h",
	MAME_DIR .. "src/devices/imagedev/microdrv.cpp",
	MAME_DIR .. "src/devices/imagedev/microdrv.h",
	MAME_DIR .. "src/devices/imagedev/midiin.cpp",
	MAME_DIR .. "src/devices/imagedev/midiin.h",
	MAME_DIR .. "src/devices/imagedev/midiout.cpp",
	MAME_DIR .. "src/devices/imagedev/midiout.h",
	MAME_DIR .. "src/devices/imagedev/picture.cpp",
	MAME_DIR .. "src/devices/imagedev/picture.h",
	MAME_DIR .. "src/devices/imagedev/printer.cpp",
	MAME_DIR .. "src/devices/imagedev/printer.h",
	MAME_DIR .. "src/devices/imagedev/snapquik.cpp",
	MAME_DIR .. "src/devices/imagedev/snapquik.h",
	MAME_DIR .. "src/devices/imagedev/wafadrive.cpp",
	MAME_DIR .. "src/devices/imagedev/wafadrive.h",
	MAME_DIR .. "src/devices/imagedev/avivideo.cpp",
	MAME_DIR .. "src/devices/imagedev/avivideo.h",
}

---------------------------------------------------
--
--@src/devices/machine/scc68070.h,MACHINES["SCC68070"] = true
---------------------------------------------------
if (MACHINES["SCC68070"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/scc68070.cpp",
		MAME_DIR .. "src/devices/machine/scc68070.h",
	}
end

---------------------------------------------------
--
--
---------------------------------------------------

if (BUSES["ATA"]~=null) or (BUSES["SCSI"]~=null) then
	MACHINES["T10"] = true
end

if (MACHINES["T10"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/t10spc.cpp",
		MAME_DIR .. "src/devices/machine/t10spc.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/output_latch.h,MACHINES["OUTPUT_LATCH"] = true
---------------------------------------------------

if (MACHINES["OUTPUT_LATCH"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/output_latch.cpp",
		MAME_DIR .. "src/devices/machine/output_latch.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/timekpr.h,MACHINES["TIMEKPR"] = true
---------------------------------------------------

if (MACHINES["TIMEKPR"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/timekpr.cpp",
		MAME_DIR .. "src/devices/machine/timekpr.h",
	}
end
