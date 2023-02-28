-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   bus.lua
--
--   Rules for building bus cores
--
---------------------------------------------------------------------------


---------------------------------------------------
--
--@src/devices/bus/scsi/scsi.h,BUSES["SCSI"] = true
---------------------------------------------------
if (BUSES["SCSI"]~=null) then
	files {
		MAME_DIR .. "src/devices/bus/scsi/scsi.h",
		MAME_DIR .. "src/devices/bus/scsi/scsihle.cpp",
		MAME_DIR .. "src/devices/bus/scsi/scsihle.h",
	}
end
