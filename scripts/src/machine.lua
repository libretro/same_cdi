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
	MAME_DIR .. "src/devices/machine/nvram.cpp",
	MAME_DIR .. "src/devices/machine/nvram.h",
	MAME_DIR .. "src/devices/machine/ram.cpp",
	MAME_DIR .. "src/devices/machine/ram.h",
	MAME_DIR .. "src/devices/machine/legscsi.cpp",
	MAME_DIR .. "src/devices/machine/legscsi.h",
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
--@src/devices/machine/74123.h,MACHINES["TTL74123"] = true
---------------------------------------------------

if (MACHINES["TTL74123"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/74123.cpp",
		MAME_DIR .. "src/devices/machine/74123.h",
		MAME_DIR .. "src/devices/machine/rescap.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/am2910.h,MACHINES["AM2910"] = true
---------------------------------------------------

if (MACHINES["AM2910"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/am2910.cpp",
		MAME_DIR .. "src/devices/machine/am2910.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/am53cf96.h,MACHINES["AM53CF96"] = true
---------------------------------------------------

if (MACHINES["AM53CF96"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/am53cf96.cpp",
		MAME_DIR .. "src/devices/machine/am53cf96.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/am79c30.h,MACHINES["AM79C30"] = true
---------------------------------------------------

if (MACHINES["AM79C30"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/am79c30.cpp",
		MAME_DIR .. "src/devices/machine/am79c30.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/am79c90.h,MACHINES["AM79C90"] = true
---------------------------------------------------

if (MACHINES["AM79C90"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/am79c90.cpp",
		MAME_DIR .. "src/devices/machine/am79c90.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/am9513.h,MACHINES["AM9513"] = true
---------------------------------------------------

if (MACHINES["AM9513"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/am9513.cpp",
		MAME_DIR .. "src/devices/machine/am9513.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/am9517a.h,MACHINES["AM9517A"] = true
---------------------------------------------------

if (MACHINES["AM9517A"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/am9517a.cpp",
		MAME_DIR .. "src/devices/machine/am9517a.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/am9519.h,MACHINES["AM9519"] = true
---------------------------------------------------

if (MACHINES["AM9519"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/am9519.cpp",
		MAME_DIR .. "src/devices/machine/am9519.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/amigafdc.h,MACHINES["AMIGAFDC"] = true
---------------------------------------------------

if (MACHINES["AMIGAFDC"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/amigafdc.cpp",
		MAME_DIR .. "src/devices/machine/amigafdc.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/applepic.h,MACHINES["APPLEPIC"] = true
---------------------------------------------------

if (MACHINES["APPLEPIC"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/applepic.cpp",
		MAME_DIR .. "src/devices/machine/applepic.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/at28c16.h,MACHINES["AT28C16"] = true
---------------------------------------------------

if (MACHINES["AT28C16"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/at28c16.cpp",
		MAME_DIR .. "src/devices/machine/at28c16.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/at28c64b.h,MACHINES["AT28C64B"] = true
---------------------------------------------------

if (MACHINES["AT28C64B"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/at28c64b.cpp",
		MAME_DIR .. "src/devices/machine/at28c64b.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/at29x.h,MACHINES["AT29X"] = true
---------------------------------------------------

if (MACHINES["AT29X"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/at29x.cpp",
		MAME_DIR .. "src/devices/machine/at29x.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/at45dbxx.h,MACHINES["AT45DBXX"] = true
---------------------------------------------------

if (MACHINES["AT45DBXX"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/at45dbxx.cpp",
		MAME_DIR .. "src/devices/machine/at45dbxx.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ataflash.h,MACHINES["ATAFLASH"] = true
---------------------------------------------------

if (MACHINES["ATAFLASH"]~=null) then
	MACHINES["PCCARD"] = true
	files {
		MAME_DIR .. "src/devices/machine/ataflash.cpp",
		MAME_DIR .. "src/devices/machine/ataflash.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/atmel_arm_aic.h,MACHINES["ARM_AIC"] = true
---------------------------------------------------

if (MACHINES["ARM_AIC"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/atmel_arm_aic.cpp",
		MAME_DIR .. "src/devices/machine/atmel_arm_aic.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ay31015.h,MACHINES["AY31015"] = true
---------------------------------------------------

if (MACHINES["AY31015"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ay31015.cpp",
		MAME_DIR .. "src/devices/machine/ay31015.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/bankdev.h,MACHINES["BANKDEV"] = true
---------------------------------------------------

if (MACHINES["BANKDEV"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/bankdev.cpp",
		MAME_DIR .. "src/devices/machine/bankdev.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/bq4847.h,MACHINES["BQ4847"] = true
---------------------------------------------------

if (MACHINES["BQ4847"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/bq4847.cpp",
		MAME_DIR .. "src/devices/machine/bq4847.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/bq48x2.h,MACHINES["BQ4852"] = true
---------------------------------------------------

if (MACHINES["BQ4852"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/bq48x2.cpp",
		MAME_DIR .. "src/devices/machine/bq48x2.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/busmouse.h,MACHINES["BUSMOUSE"] = true
---------------------------------------------------

if (MACHINES["BUSMOUSE"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/busmouse.cpp",
		MAME_DIR .. "src/devices/machine/busmouse.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/cdp1852.h,MACHINES["CDP1852"] = true
---------------------------------------------------

if (MACHINES["CDP1852"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/cdp1852.cpp",
		MAME_DIR .. "src/devices/machine/cdp1852.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/cdp1871.h,MACHINES["CDP1871"] = true
---------------------------------------------------

if (MACHINES["CDP1871"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/cdp1871.cpp",
		MAME_DIR .. "src/devices/machine/cdp1871.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/cdp1879.h,MACHINES["CDP1879"] = true
---------------------------------------------------

if (MACHINES["CDP1879"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/cdp1879.cpp",
		MAME_DIR .. "src/devices/machine/cdp1879.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ch376.h,MACHINES["CH376"] = true
---------------------------------------------------

if (MACHINES["CH376"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ch376.cpp",
		MAME_DIR .. "src/devices/machine/ch376.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/chessmachine.h,MACHINES["CHESSMACHINE"] = true
---------------------------------------------------

if (MACHINES["CHESSMACHINE"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/chessmachine.cpp",
		MAME_DIR .. "src/devices/machine/chessmachine.h",
	}
end


---------------------------------------------------
--
--@src/devices/machine/com52c50.h,MACHINES["COM52C50"] = true
---------------------------------------------------

if (MACHINES["COM52C50"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/com52c50.cpp",
		MAME_DIR .. "src/devices/machine/com52c50.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/com8116.h,MACHINES["COM8116"] = true
---------------------------------------------------

if (MACHINES["COM8116"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/com8116.cpp",
		MAME_DIR .. "src/devices/machine/com8116.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/cs4031.h,MACHINES["CS4031"] = true
---------------------------------------------------

if (MACHINES["CS4031"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/cs4031.cpp",
		MAME_DIR .. "src/devices/machine/cs4031.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/cs8221.h,MACHINES["CS8221"] = true
---------------------------------------------------

if (MACHINES["CS8221"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/cs8221.cpp",
		MAME_DIR .. "src/devices/machine/cs8221.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/cs8900a.h,MACHINES["CS8900A"] = true
---------------------------------------------------

if (MACHINES["CS8900A"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/cs8900a.cpp",
		MAME_DIR .. "src/devices/machine/cs8900a.h",
	}
end


---------------------------------------------------
--
--@src/devices/machine/cxd1095.h,MACHINES["CXD1095"] = true
---------------------------------------------------

if (MACHINES["CXD1095"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/cxd1095.cpp",
		MAME_DIR .. "src/devices/machine/cxd1095.h",
	}
end

--@src/devices/machine/dl11.h,MACHINES["DL11"] = true
---------------------------------------------------

if (MACHINES["DL11"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/dl11.cpp",
		MAME_DIR .. "src/devices/machine/dl11.h",
	}
end

--@src/devices/machine/ds1204.h,MACHINES["DS1204"] = true
---------------------------------------------------

if (MACHINES["DS1204"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds1204.cpp",
		MAME_DIR .. "src/devices/machine/ds1204.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds1205.h,MACHINES["DS1205"] = true
---------------------------------------------------

if (MACHINES["DS1205"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds1205.cpp",
		MAME_DIR .. "src/devices/machine/ds1205.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds1302.h,MACHINES["DS1302"] = true
---------------------------------------------------

if (MACHINES["DS1302"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds1302.cpp",
		MAME_DIR .. "src/devices/machine/ds1302.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds1315.h,MACHINES["DS1315"] = true
---------------------------------------------------

if (MACHINES["DS1315"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds1315.cpp",
		MAME_DIR .. "src/devices/machine/ds1315.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds1386.h,MACHINES["DS1386"] = true
---------------------------------------------------

if (MACHINES["DS1386"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds1386.cpp",
		MAME_DIR .. "src/devices/machine/ds1386.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds17x85.h,MACHINES["DS17X85"] = true
---------------------------------------------------

if (MACHINES["DS17X85"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds17x85.cpp",
		MAME_DIR .. "src/devices/machine/ds17x85.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds1994.h,MACHINES["DS1994"] = true
---------------------------------------------------

if (MACHINES["DS1994"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds1994.cpp",
		MAME_DIR .. "src/devices/machine/ds1994.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds2401.h,MACHINES["DS2401"] = true
---------------------------------------------------

if (MACHINES["DS2401"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds2401.cpp",
		MAME_DIR .. "src/devices/machine/ds2401.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds2404.h,MACHINES["DS2404"] = true
---------------------------------------------------

if (MACHINES["DS2404"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds2404.cpp",
		MAME_DIR .. "src/devices/machine/ds2404.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds6417.h,MACHINES["DS6417"] = true
---------------------------------------------------

if (MACHINES["DS6417"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds6417.cpp",
		MAME_DIR .. "src/devices/machine/ds6417.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds75160a.h,MACHINES["DS75160A"] = true
---------------------------------------------------

if (MACHINES["DS75160A"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds75160a.cpp",
		MAME_DIR .. "src/devices/machine/ds75160a.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds75161a.h,MACHINES["DS75161A"] = true
---------------------------------------------------

if (MACHINES["DS75161A"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds75161a.cpp",
		MAME_DIR .. "src/devices/machine/ds75161a.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ds8874.h,MACHINES["DS8874"] = true
---------------------------------------------------

if (MACHINES["DS8874"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ds8874.cpp",
		MAME_DIR .. "src/devices/machine/ds8874.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/e0516.h,MACHINES["E0516"] = true
---------------------------------------------------

if (MACHINES["E0516"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/e0516.cpp",
		MAME_DIR .. "src/devices/machine/e0516.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/e05a03.h,MACHINES["E05A03"] = true
---------------------------------------------------

if (MACHINES["E05A03"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/e05a03.cpp",
		MAME_DIR .. "src/devices/machine/e05a03.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/e05a30.h,MACHINES["E05A30"] = true
---------------------------------------------------

if (MACHINES["E05A30"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/e05a30.cpp",
		MAME_DIR .. "src/devices/machine/e05a30.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/eeprom.h,MACHINES["EEPROMDEV"] = true
--@src/devices/machine/eepromser.h,MACHINES["EEPROMDEV"] = true
--@src/devices/machine/eeprompar.h,MACHINES["EEPROMDEV"] = true
---------------------------------------------------

if (MACHINES["EEPROMDEV"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/eeprom.cpp",
		MAME_DIR .. "src/devices/machine/eeprom.h",
		MAME_DIR .. "src/devices/machine/eepromser.cpp",
		MAME_DIR .. "src/devices/machine/eepromser.h",
		MAME_DIR .. "src/devices/machine/eeprompar.cpp",
		MAME_DIR .. "src/devices/machine/eeprompar.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/er1400.h,MACHINES["ER1400"] = true
---------------------------------------------------

if (MACHINES["ER1400"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/er1400.cpp",
		MAME_DIR .. "src/devices/machine/er1400.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/er2055.h,MACHINES["ER2055"] = true
---------------------------------------------------

if (MACHINES["ER2055"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/er2055.cpp",
		MAME_DIR .. "src/devices/machine/er2055.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/exorterm.h,MACHINES["EXORTERM"] = true
---------------------------------------------------

if (MACHINES["EXORTERM"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/exorterm.cpp",
		MAME_DIR .. "src/devices/machine/exorterm.h",
	}

	dependency {
		{ MAME_DIR .. "src/devices/machine/exorterm.cpp", GEN_DIR .. "emu/layout/exorterm155.lh" },
	}

	custombuildtask {
		layoutbuildtask("emu/layout", "exorterm155"),
	}
end

---------------------------------------------------
--
--@src/devices/machine/f3853.h,MACHINES["F3853"] = true
---------------------------------------------------

if (MACHINES["F3853"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/f3853.cpp",
		MAME_DIR .. "src/devices/machine/f3853.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/f4702.h,MACHINES["F4702"] = true
---------------------------------------------------

if (MACHINES["F4702"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/f4702.cpp",
		MAME_DIR .. "src/devices/machine/f4702.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/fga002.h,MACHINES["FGA002"] = true
---------------------------------------------------

if (MACHINES["FGA002"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/fga002.cpp",
		MAME_DIR .. "src/devices/machine/fga002.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/gt913_io.h,MACHINES["GT913"] = true
--@src/devices/machine/gt913_kbd.h,MACHINES["GT913"] = true
--@src/devices/machine/gt913_snd.h,MACHINES["GT913"] = true
---------------------------------------------------

if (MACHINES["GT913"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/gt913_io.cpp",
		MAME_DIR .. "src/devices/machine/gt913_io.h",
		MAME_DIR .. "src/devices/machine/gt913_kbd.cpp",
		MAME_DIR .. "src/devices/machine/gt913_kbd.h",
		MAME_DIR .. "src/devices/machine/gt913_snd.cpp",
		MAME_DIR .. "src/devices/machine/gt913_snd.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/hd63450.h,MACHINES["HD63450"] = true
---------------------------------------------------

if (MACHINES["HD63450"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/hd63450.cpp",
		MAME_DIR .. "src/devices/machine/hd63450.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/hd64610.h,MACHINES["HD64610"] = true
---------------------------------------------------

if (MACHINES["HD64610"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/hd64610.cpp",
		MAME_DIR .. "src/devices/machine/hd64610.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/hp_dc100_tape.h,MACHINES["HP_DC100_TAPE"] = true
---------------------------------------------------

if (MACHINES["HP_DC100_TAPE"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/hp_dc100_tape.cpp",
		MAME_DIR .. "src/devices/machine/hp_dc100_tape.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/hp_taco.h,MACHINES["HP_TACO"] = true
---------------------------------------------------

if (MACHINES["HP_TACO"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/hp_taco.cpp",
		MAME_DIR .. "src/devices/machine/hp_taco.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/1ma6.h,MACHINES["1MA6"] = true
---------------------------------------------------

if (MACHINES["1MA6"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/1ma6.cpp",
		MAME_DIR .. "src/devices/machine/1ma6.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/1mb5.h,MACHINES["1MB5"] = true
---------------------------------------------------

if (MACHINES["1MB5"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/1mb5.cpp",
		MAME_DIR .. "src/devices/machine/1mb5.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i2cmem.h,MACHINES["I2CMEM"] = true
---------------------------------------------------

if (MACHINES["I2CMEM"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i2cmem.cpp",
		MAME_DIR .. "src/devices/machine/i2cmem.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i7220.h,MACHINES["I7220"] = true
---------------------------------------------------

if (MACHINES["I7220"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i7220.cpp",
		MAME_DIR .. "src/devices/machine/i7220.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8087.h,MACHINES["I8087"] = true
---------------------------------------------------

if (MACHINES["I8087"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8087.cpp",
		MAME_DIR .. "src/devices/machine/i8087.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8155.h,MACHINES["I8155"] = true
---------------------------------------------------

if (MACHINES["I8155"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8155.cpp",
		MAME_DIR .. "src/devices/machine/i8155.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8212.h,MACHINES["I8212"] = true
---------------------------------------------------

if (MACHINES["I8212"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8212.cpp",
		MAME_DIR .. "src/devices/machine/i8212.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8214.h,MACHINES["I8214"] = true
---------------------------------------------------

if (MACHINES["I8214"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8214.cpp",
		MAME_DIR .. "src/devices/machine/i8214.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i82355.h,MACHINES["I82355"] = true
---------------------------------------------------

if (MACHINES["I82355"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i82355.cpp",
		MAME_DIR .. "src/devices/machine/i82355.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8243.h,MACHINES["I8243"] = true
---------------------------------------------------

if (MACHINES["I8243"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8243.cpp",
		MAME_DIR .. "src/devices/machine/i8243.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8251.h,MACHINES["I8251"] = true
---------------------------------------------------

if (MACHINES["I8251"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8251.cpp",
		MAME_DIR .. "src/devices/machine/i8251.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8257.h,MACHINES["I8257"] = true
---------------------------------------------------

if (MACHINES["I8257"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8257.cpp",
		MAME_DIR .. "src/devices/machine/i8257.h",
	}
end


---------------------------------------------------
--
--@src/devices/machine/i8271.h,MACHINES["I8271"] = true
---------------------------------------------------

if (MACHINES["I8271"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8271.cpp",
		MAME_DIR .. "src/devices/machine/i8271.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8279.h,MACHINES["I8279"] = true
---------------------------------------------------

if (MACHINES["I8279"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8279.cpp",
		MAME_DIR .. "src/devices/machine/i8279.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i8355.h,MACHINES["I8355"] = true
---------------------------------------------------

if (MACHINES["I8355"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8355.cpp",
		MAME_DIR .. "src/devices/machine/i8355.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/i80130.h,MACHINES["I80130"] = true
---------------------------------------------------

if (MACHINES["I80130"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i80130.cpp",
		MAME_DIR .. "src/devices/machine/i80130.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/icm7170.h,MACHINES["ICM7170"] = true
---------------------------------------------------

if (MACHINES["ICM7170"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/icm7170.cpp",
		MAME_DIR .. "src/devices/machine/icm7170.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ibm21s850.h,MACHINES["IBM21S850"] = true
---------------------------------------------------

if (MACHINES["IBM21S850"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ibm21s850.cpp",
		MAME_DIR .. "src/devices/machine/ibm21s850.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/idectrl.h,MACHINES["IDECTRL"] = true
--@src/devices/machine/vt83c461.h,MACHINES["IDECTRL"] = true
---------------------------------------------------

if (MACHINES["IDECTRL"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/idectrl.cpp",
		MAME_DIR .. "src/devices/machine/idectrl.h",
		MAME_DIR .. "src/devices/machine/vt83c461.cpp",
		MAME_DIR .. "src/devices/machine/vt83c461.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ie15.h,MACHINES["IE15"] = true
---------------------------------------------------

if (MACHINES["IE15"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ie15.cpp",
		MAME_DIR .. "src/devices/machine/ie15.h",
		MAME_DIR .. "src/devices/machine/ie15_kbd.cpp",
		MAME_DIR .. "src/devices/machine/ie15_kbd.h",
	}

	dependency {
		{ MAME_DIR .. "src/devices/machine/ie15.cpp", GEN_DIR .. "emu/layout/ie15.lh" },
	}

	custombuildtask {
		layoutbuildtask("emu/layout", "ie15"),
	}
end

---------------------------------------------------
--
--@src/devices/machine/im6402.h,MACHINES["IM6402"] = true
---------------------------------------------------

if (MACHINES["IM6402"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/im6402.cpp",
		MAME_DIR .. "src/devices/machine/im6402.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ins8154.h,MACHINES["INS8154"] = true
---------------------------------------------------

if (MACHINES["INS8154"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ins8154.cpp",
		MAME_DIR .. "src/devices/machine/ins8154.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ins8250.h,MACHINES["INS8250"] = true
---------------------------------------------------

if (MACHINES["INS8250"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ins8250.cpp",
		MAME_DIR .. "src/devices/machine/ins8250.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/intelfsh.h,MACHINES["INTELFLASH"] = true
---------------------------------------------------

if (MACHINES["INTELFLASH"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/intelfsh.cpp",
		MAME_DIR .. "src/devices/machine/intelfsh.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/jvsdev.h,MACHINES["JVS"] = true
--@src/devices/machine/jvshost.h,MACHINES["JVS"] = true
---------------------------------------------------

if (MACHINES["JVS"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/jvsdev.cpp",
		MAME_DIR .. "src/devices/machine/jvsdev.h",
		MAME_DIR .. "src/devices/machine/jvshost.cpp",
		MAME_DIR .. "src/devices/machine/jvshost.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/k033906.h,MACHINES["K033906"] = true
---------------------------------------------------

if (MACHINES["K033906"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/k033906.cpp",
		MAME_DIR .. "src/devices/machine/k033906.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/k053252.h,MACHINES["K053252"] = true
---------------------------------------------------

if (MACHINES["K053252"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/k053252.cpp",
		MAME_DIR .. "src/devices/machine/k053252.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/k056230.h,MACHINES["K056230"] = true
---------------------------------------------------

if (MACHINES["K056230"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/k056230.cpp",
		MAME_DIR .. "src/devices/machine/k056230.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/m950x0.h,MACHINES["M950X0"] = true
---------------------------------------------------

if (MACHINES["M950X0"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/m950x0.cpp",
		MAME_DIR .. "src/devices/machine/m950x0.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mm5740.h,MACHINES["MM5740"] = true
---------------------------------------------------

if (MACHINES["MM5740"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mm5740.cpp",
		MAME_DIR .. "src/devices/machine/mm5740.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/kb3600.h,MACHINES["KB3600"] = true
---------------------------------------------------

if (MACHINES["KB3600"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/kb3600.cpp",
		MAME_DIR .. "src/devices/machine/kb3600.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/kr2376.h,MACHINES["KR2376"] = true
---------------------------------------------------

if (MACHINES["KR2376"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/kr2376.cpp",
		MAME_DIR .. "src/devices/machine/kr2376.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/latch8.h,MACHINES["LATCH8"] = true
---------------------------------------------------

if (MACHINES["LATCH8"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/latch8.cpp",
		MAME_DIR .. "src/devices/machine/latch8.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/lc89510.h,MACHINES["LC89510"] = true
---------------------------------------------------

if (MACHINES["LC89510"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/lc89510.cpp",
		MAME_DIR .. "src/devices/machine/lc89510.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ldpr8210.h,MACHINES["LDPR8210"] = true
---------------------------------------------------

if (MACHINES["LDPR8210"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ldpr8210.cpp",
		MAME_DIR .. "src/devices/machine/ldpr8210.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ldstub.h,MACHINES["LDSTUB"] = true
---------------------------------------------------

if (MACHINES["LDSTUB"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ldstub.cpp",
		MAME_DIR .. "src/devices/machine/ldstub.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ldv1000.h,MACHINES["LDV1000"] = true
---------------------------------------------------

if (MACHINES["LDV1000"]~=null) then
	MACHINES["Z80CTC"] = true
	MACHINES["I8255"] = true
	files {
		MAME_DIR .. "src/devices/machine/ldv1000.cpp",
		MAME_DIR .. "src/devices/machine/ldv1000.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ldp1000.h,MACHINES["LDP1000"] = true
---------------------------------------------------

if (MACHINES["LDP1000"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ldp1000.cpp",
		MAME_DIR .. "src/devices/machine/ldp1000.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ldp1450.h,MACHINES["LDP1450"] = true
---------------------------------------------------

if (MACHINES["LDP1450"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ldp1450.cpp",
		MAME_DIR .. "src/devices/machine/ldp1450.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ldvp931.h,MACHINES["LDVP931"] = true
---------------------------------------------------

if (MACHINES["LDVP931"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ldvp931.cpp",
		MAME_DIR .. "src/devices/machine/ldvp931.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/lh5810.h,MACHINES["LH5810"] = true
---------------------------------------------------

if (MACHINES["LH5810"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/lh5810.cpp",
		MAME_DIR .. "src/devices/machine/lh5810.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/linflash.h,MACHINES["LINFLASH"] = true
---------------------------------------------------

if (MACHINES["LINFLASH"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/linflash.cpp",
		MAME_DIR .. "src/devices/machine/linflash.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/locomo.h,MACHINES["LOCOMO"] = true
---------------------------------------------------

if (MACHINES["LOCOMO"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/locomo.cpp",
		MAME_DIR .. "src/devices/machine/locomo.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/m3002.h,MACHINES["M3002"] = true
---------------------------------------------------

if (MACHINES["M3002"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/m3002.cpp",
		MAME_DIR .. "src/devices/machine/m3002.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/m68sfdc.h,MACHINES["M68SFDC"] = true
---------------------------------------------------

if (MACHINES["M68SFDC"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/m68sfdc.cpp",
		MAME_DIR .. "src/devices/machine/m68sfdc.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/m6m80011ap.h,MACHINES["M6M80011AP"] = true
---------------------------------------------------

if (MACHINES["M6M80011AP"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/m6m80011ap.cpp",
		MAME_DIR .. "src/devices/machine/m6m80011ap.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb14241.h,MACHINES["MB14241"] = true
---------------------------------------------------

if (MACHINES["MB14241"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb14241.cpp",
		MAME_DIR .. "src/devices/machine/mb14241.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb3773.h,MACHINES["MB3773"] = true
---------------------------------------------------

if (MACHINES["MB3773"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb3773.cpp",
		MAME_DIR .. "src/devices/machine/mb3773.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb8421.h,MACHINES["MB8421"] = true
---------------------------------------------------

if (MACHINES["MB8421"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb8421.cpp",
		MAME_DIR .. "src/devices/machine/mb8421.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb87030.h,MACHINES["MB87030"] = true
---------------------------------------------------

if (MACHINES["MB87030"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb87030.cpp",
		MAME_DIR .. "src/devices/machine/mb87030.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb87078.h,MACHINES["MB87078"] = true
---------------------------------------------------

if (MACHINES["MB87078"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb87078.cpp",
		MAME_DIR .. "src/devices/machine/mb87078.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb8795.h,MACHINES["MB8795"] = true
---------------------------------------------------

if (MACHINES["MB8795"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb8795.cpp",
		MAME_DIR .. "src/devices/machine/mb8795.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb89352.h,MACHINES["MB89352"] = true
---------------------------------------------------

if (MACHINES["MB89352"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb89352.cpp",
		MAME_DIR .. "src/devices/machine/mb89352.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb89371.h,MACHINES["MB89371"] = true
---------------------------------------------------

if (MACHINES["MB89371"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb89371.cpp",
		MAME_DIR .. "src/devices/machine/mb89371.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mb89374.h,MACHINES["MB89374"] = true
---------------------------------------------------

if (MACHINES["MB89374"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mb89374.cpp",
		MAME_DIR .. "src/devices/machine/mb89374.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc146818.h,MACHINES["MC146818"] = true
---------------------------------------------------

if (MACHINES["MC146818"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc146818.cpp",
		MAME_DIR .. "src/devices/machine/mc146818.h",
		MAME_DIR .. "src/devices/machine/ds128x.cpp",
		MAME_DIR .. "src/devices/machine/ds128x.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc14411.h,MACHINES["MC14411"] = true
---------------------------------------------------

if (MACHINES["MC14411"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc14411.cpp",
		MAME_DIR .. "src/devices/machine/mc14411.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc6843.h,MACHINES["MC6843"] = true
---------------------------------------------------

if (MACHINES["MC6843"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc6843.cpp",
		MAME_DIR .. "src/devices/machine/mc6843.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc6844.h,MACHINES["MC6844"] = true
---------------------------------------------------

if (MACHINES["MC6844"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc6844.cpp",
		MAME_DIR .. "src/devices/machine/mc6844.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc6846.h,MACHINES["MC6846"] = true
---------------------------------------------------

if (MACHINES["MC6846"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc6846.cpp",
		MAME_DIR .. "src/devices/machine/mc6846.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc6852.h,MACHINES["MC6852"] = true
---------------------------------------------------

if (MACHINES["MC6852"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc6852.cpp",
		MAME_DIR .. "src/devices/machine/mc6852.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc6854.h,MACHINES["MC6854"] = true
---------------------------------------------------

if (MACHINES["MC6854"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc6854.cpp",
		MAME_DIR .. "src/devices/machine/mc6854.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc68328.h,MACHINES["MC68328"] = true
---------------------------------------------------

if (MACHINES["MC68328"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc68328.cpp",
		MAME_DIR .. "src/devices/machine/mc68328.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mc68901.h,MACHINES["MC68901"] = true
---------------------------------------------------

if (MACHINES["MC68901"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mc68901.cpp",
		MAME_DIR .. "src/devices/machine/mc68901.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mccs1850.h,MACHINES["MCCS1850"] = true
---------------------------------------------------

if (MACHINES["MCCS1850"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mccs1850.cpp",
		MAME_DIR .. "src/devices/machine/mccs1850.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/68307.h,MACHINES["M68307"] = true
---------------------------------------------------

if (MACHINES["M68307"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/68307.cpp",
		MAME_DIR .. "src/devices/machine/68307.h",
		MAME_DIR .. "src/devices/machine/68307sim.cpp",
		MAME_DIR .. "src/devices/machine/68307sim.h",
		MAME_DIR .. "src/devices/machine/68307bus.cpp",
		MAME_DIR .. "src/devices/machine/68307bus.h",
		MAME_DIR .. "src/devices/machine/68307tmu.cpp",
		MAME_DIR .. "src/devices/machine/68307tmu.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/68340.h,MACHINES["M68340"] = true
---------------------------------------------------

if (MACHINES["M68340"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/68340.cpp",
		MAME_DIR .. "src/devices/machine/68340.h",
		MAME_DIR .. "src/devices/machine/68340sim.cpp",
		MAME_DIR .. "src/devices/machine/68340sim.h",
		MAME_DIR .. "src/devices/machine/68340dma.cpp",
		MAME_DIR .. "src/devices/machine/68340dma.h",
		MAME_DIR .. "src/devices/machine/68340ser.cpp",
		MAME_DIR .. "src/devices/machine/68340ser.h",
		MAME_DIR .. "src/devices/machine/68340tmu.cpp",
		MAME_DIR .. "src/devices/machine/68340tmu.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mcf5206e.h,MACHINES["MCF5206E"] = true
---------------------------------------------------

if (MACHINES["MCF5206E"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mcf5206e.cpp",
		MAME_DIR .. "src/devices/machine/mcf5206e.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/pckeybrd.h,MACHINES["PCKEYBRD"] = true
---------------------------------------------------

if (MACHINES["PCKEYBRD"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/pckeybrd.cpp",
		MAME_DIR .. "src/devices/machine/pckeybrd.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/s3c2400.h,MACHINES["S3C24XX"] = true
--@src/devices/machine/s3c2410.h,MACHINES["S3C24XX"] = true
--@src/devices/machine/s3c2440.h,MACHINES["S3C24XX"] = true
---------------------------------------------------

if (MACHINES["S3C24XX"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/s3c2400.cpp",
		MAME_DIR .. "src/devices/machine/s3c2400.h",
		MAME_DIR .. "src/devices/machine/s3c2410.cpp",
		MAME_DIR .. "src/devices/machine/s3c2410.h",
		MAME_DIR .. "src/devices/machine/s3c2440.cpp",
		MAME_DIR .. "src/devices/machine/s3c2440.h",
		MAME_DIR .. "src/devices/machine/s3c24xx.cpp",
		MAME_DIR .. "src/devices/machine/s3c24xx.h",
		MAME_DIR .. "src/devices/machine/s3c24xx.hxx",
	}
end

---------------------------------------------------
--
--@src/devices/machine/s3c44b0.h,MACHINES["S3C44B0"] = true
---------------------------------------------------

if (MACHINES["S3C44B0"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/s3c44b0.cpp",
		MAME_DIR .. "src/devices/machine/s3c44b0.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/sa1110.h,MACHINES["SA1110"] = true
---------------------------------------------------

if (MACHINES["SA1110"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/sa1110.cpp",
		MAME_DIR .. "src/devices/machine/sa1110.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/sa1111.h,MACHINES["SA1111"] = true
---------------------------------------------------

if (MACHINES["SA1111"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/sa1111.cpp",
		MAME_DIR .. "src/devices/machine/sa1111.h",
	}
end

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
		MAME_DIR .. "src/devices/machine/t10mmc.cpp",
		MAME_DIR .. "src/devices/machine/t10mmc.h",
		MAME_DIR .. "src/devices/machine/t10sbc.cpp",
		MAME_DIR .. "src/devices/machine/t10sbc.h",
		MAME_DIR .. "src/devices/machine/t10spc.cpp",
		MAME_DIR .. "src/devices/machine/t10spc.h",
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

---------------------------------------------------
--
--@src/devices/machine/i8255.h,MACHINES["I8255"] = true
---------------------------------------------------

if (MACHINES["I8255"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/i8255.cpp",
		MAME_DIR .. "src/devices/machine/i8255.h",
		MAME_DIR .. "src/devices/machine/mb89363b.cpp",
		MAME_DIR .. "src/devices/machine/mb89363b.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ncr5380.h,MACHINES["NCR5380"] = true
---------------------------------------------------

if (MACHINES["NCR5380"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ncr5380.cpp",
		MAME_DIR .. "src/devices/machine/ncr5380.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/ncr5390.h,MACHINES["NCR5390"] = true
---------------------------------------------------

if (MACHINES["NCR5390"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/ncr5390.cpp",
		MAME_DIR .. "src/devices/machine/ncr5390.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mm58167.h,MACHINES["MM58167"] = true
---------------------------------------------------

if (MACHINES["MM58167"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mm58167.cpp",
		MAME_DIR .. "src/devices/machine/mm58167.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/mm58174.h,MACHINES["MM58174"] = true
---------------------------------------------------

if (MACHINES["MM58174"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/mm58174.cpp",
		MAME_DIR .. "src/devices/machine/mm58174.h",
	}
end


---------------------------------------------------
--
--@src/devices/machine/dp8390.h,MACHINES["DP8390"] = true
---------------------------------------------------

if (MACHINES["DP8390"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/dp8390.cpp",
		MAME_DIR .. "src/devices/machine/dp8390.h",
	}
end

---------------------------------------------------
--
--@src/devices/machine/dp83932c.h,MACHINES["DP83932C"] = true
---------------------------------------------------

if (MACHINES["DP83932C"]~=null) then
	files {
		MAME_DIR .. "src/devices/machine/dp83932c.cpp",
		MAME_DIR .. "src/devices/machine/dp83932c.h",
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
