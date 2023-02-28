-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   sound.lua
--
--   Rules for building sound cores
--
----------------------------------------------------------------------------

files {
	MAME_DIR .. "src/devices/sound/bbd.cpp",
	MAME_DIR .. "src/devices/sound/bbd.h",
	MAME_DIR .. "src/devices/sound/flt_biquad.cpp",
	MAME_DIR .. "src/devices/sound/flt_biquad.h",
	MAME_DIR .. "src/devices/sound/flt_rc.cpp",
	MAME_DIR .. "src/devices/sound/flt_rc.h",
	MAME_DIR .. "src/devices/sound/flt_vol.cpp",
	MAME_DIR .. "src/devices/sound/flt_vol.h",
	MAME_DIR .. "src/devices/sound/mixer.cpp",
	MAME_DIR .. "src/devices/sound/mixer.h",
	MAME_DIR .. "src/devices/sound/samples.cpp",
	MAME_DIR .. "src/devices/sound/samples.h",
}

---------------------------------------------------
-- DACs
--@src/devices/sound/dmadac.h,SOUNDS["DMADAC"] = true
--@src/devices/sound/beep.h,SOUNDS["BEEP"] = true
---------------------------------------------------

if (SOUNDS["DMADAC"]~=null) then
	files {
		MAME_DIR .. "src/devices/sound/dmadac.cpp",
		MAME_DIR .. "src/devices/sound/dmadac.h",
	}
end

if (SOUNDS["BEEP"]~=null) then
	files {
		MAME_DIR .. "src/devices/sound/beep.cpp",
		MAME_DIR .. "src/devices/sound/beep.h",
	}
end

---------------------------------------------------
-- CD audio
--@src/devices/sound/cdda.h,SOUNDS["CDDA"] = true
---------------------------------------------------

if (SOUNDS["CDDA"]~=null) then
	files {
		MAME_DIR .. "src/devices/sound/cdda.cpp",
		MAME_DIR .. "src/devices/sound/cdda.h",
	}
end
