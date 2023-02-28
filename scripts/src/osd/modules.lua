-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   modules.lua
--
--   Rules for the building of modules
--
---------------------------------------------------------------------------

function string.starts(String,Start)
	return string.sub(String,1,string.len(Start))==Start
end

function addlibfromstring(str)
	if (str==nil) then return  end
	for w in str:gmatch("%S+") do
		if string.starts(w,"-l")==true then
			links {
				string.sub(w,3)
			}
		end
	end
end

function addoptionsfromstring(str)
	if (str==nil) then return  end
	for w in str:gmatch("%S+") do
		if string.starts(w,"-l")==false then
			linkoptions {
				w
			}
		end
	end
end

function pkgconfigcmd()
	local pkgconfig = os.getenv("PKG_CONFIG")
	if pkgconfig == nil then
		return "pkg-config"
	end
	return pkgconfig
end

function osdmodulesbuild()

	removeflags {
		"SingleOutputDir",
	}

	files {
		MAME_DIR .. "src/osd/osdnet.cpp",
		MAME_DIR .. "src/osd/osdnet.h",
		MAME_DIR .. "src/osd/watchdog.cpp",
		MAME_DIR .. "src/osd/watchdog.h",
		MAME_DIR .. "src/osd/modules/debugger/debug_module.h",
		MAME_DIR .. "src/osd/modules/font/font_module.h",
		MAME_DIR .. "src/osd/modules/midi/midi_module.h",
		MAME_DIR .. "src/osd/modules/netdev/netdev_module.h",
		MAME_DIR .. "src/osd/modules/sound/sound_module.h",
		MAME_DIR .. "src/osd/modules/diagnostics/diagnostics_module.h",
		MAME_DIR .. "src/osd/modules/monitor/monitor_module.h",
		MAME_DIR .. "src/osd/modules/lib/osdobj_common.cpp",
		MAME_DIR .. "src/osd/modules/lib/osdobj_common.h",
		MAME_DIR .. "src/osd/modules/diagnostics/none.cpp",
		MAME_DIR .. "src/osd/modules/debugger/none.cpp",
		MAME_DIR .. "src/osd/modules/font/font_none.cpp",
		MAME_DIR .. "src/osd/modules/netdev/taptun.cpp",
		MAME_DIR .. "src/osd/modules/netdev/pcap.cpp",
		MAME_DIR .. "src/osd/modules/netdev/none.cpp",
		MAME_DIR .. "src/osd/modules/midi/portmidi.cpp",
		MAME_DIR .. "src/osd/modules/midi/none.cpp",
		MAME_DIR .. "src/osd/modules/sound/none.cpp",
		MAME_DIR .. "src/osd/modules/input/input_module.h",
		MAME_DIR .. "src/osd/modules/input/input_common.cpp",
		MAME_DIR .. "src/osd/modules/input/input_common.h",
		MAME_DIR .. "src/osd/modules/input/input_none.cpp",
		MAME_DIR .. "src/osd/modules/output/output_module.h",
		MAME_DIR .. "src/osd/modules/output/none.cpp",
		MAME_DIR .. "src/osd/modules/monitor/monitor_common.h",
		MAME_DIR .. "src/osd/modules/monitor/monitor_common.cpp",
	}
	includedirs {
		ext_includedir("asio"),
	}

	if _OPTIONS["gcc"]~=nil and string.find(_OPTIONS["gcc"], "clang") then
		buildoptions {
			"-Wno-unused-private-field",
		}
	end

	if _OPTIONS["targetos"]=="windows" then
		includedirs {
			MAME_DIR .. "3rdparty/winpcap/Include",
			MAME_DIR .. "3rdparty/compat/mingw",
		}

		includedirs {
			MAME_DIR .. "3rdparty/compat/winsdk-override",
		}
	end

	defines {
		"__STDC_LIMIT_MACROS",
		"__STDC_FORMAT_MACROS",
		"__STDC_CONSTANT_MACROS",
	}

	files {
		MAME_DIR .. "src/osd/modules/render/binpacker.cpp",
	}
	includedirs {
		ext_includedir("rapidjson")
	}

	if _OPTIONS["NO_USE_MIDI"]=="1" then
		defines {
			"NO_USE_MIDI",
		}
	else
		includedirs {
			ext_includedir("portmidi"),
		}
	end
end


function osdmodulestargetconf()

	if _OPTIONS["NO_USE_MIDI"]~="1" then
		if _OPTIONS["targetos"]=="linux" then
			local str = backtick(pkgconfigcmd() .. " --libs alsa")
			addlibfromstring(str)
			addoptionsfromstring(str)
		elseif _OPTIONS["targetos"]=="macosx" then
			links {
				"CoreMIDI.framework",
			}
		end
	end

	if _OPTIONS["targetos"]=="windows" then
		links {
			"gdi32",
			"dsound",
			"dxguid",
			"oleaut32",
			"winmm",
		}
	elseif _OPTIONS["targetos"]=="macosx" then
		links {
			"AudioUnit.framework",
			"AudioToolbox.framework",
			"CoreServices.framework",
		}
	end
end


newoption {
	trigger = "USE_TAPTUN",
	description = "Include tap/tun network module",
	allowed = {
		{ "0",  "Don't include tap/tun network module" },
		{ "1",  "Include tap/tun network module" },
	},
}

newoption {
	trigger = "USE_PCAP",
	description = "Include pcap network module",
	allowed = {
		{ "0",  "Don't include pcap network module" },
		{ "1",  "Include pcap network module" },
	},
}

newoption {
	trigger = "NO_USE_MIDI",
	description = "Disable MIDI I/O",
	allowed = {
		{ "0",  "Enable MIDI"  },
		{ "1",  "Disable MIDI" },
	},
}

if not _OPTIONS["NO_USE_MIDI"] then
	if _OPTIONS["targetos"]=="freebsd" or _OPTIONS["targetos"]=="openbsd" or _OPTIONS["targetos"]=="netbsd" or _OPTIONS["targetos"]=="solaris" or _OPTIONS["targetos"]=="haiku" or _OPTIONS["targetos"] == "asmjs" then
		_OPTIONS["NO_USE_MIDI"] = "1"
	else
		_OPTIONS["NO_USE_MIDI"] = "0"
	end
end

newoption {
	trigger = "MODERN_WIN_API",
	description = "Use Modern Windows APIs",
	allowed = {
		{ "0",  "Use classic Windows APIs - allows support for XP and later"   },
		{ "1",  "Use Modern Windows APIs - support for Windows 8.1 and later"  },
	},
}

if not _OPTIONS["USE_TAPTUN"] then
	if _OPTIONS["targetos"]=="linux" or _OPTIONS["targetos"]=="windows" then
		_OPTIONS["USE_TAPTUN"] = "1"
	else
		_OPTIONS["USE_TAPTUN"] = "0"
	end
end

if not _OPTIONS["USE_PCAP"] then
	if _OPTIONS["targetos"]=="macosx" or _OPTIONS["targetos"]=="netbsd" then
		_OPTIONS["USE_PCAP"] = "1"
	else
		_OPTIONS["USE_PCAP"] = "0"
	end
end
