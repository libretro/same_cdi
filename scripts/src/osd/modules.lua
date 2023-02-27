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
			MAME_DIR .. "3rdparty/portaudio/include",
		}

		includedirs {
			MAME_DIR .. "3rdparty/compat/winsdk-override",
		}
	end

	if _OPTIONS["NO_OPENGL"]=="1" then
		defines {
			"USE_OPENGL=0",
		}
	else
		files {
			MAME_DIR .. "src/osd/modules/render/drawogl.cpp",
			MAME_DIR .. "src/osd/modules/opengl/gl_shader_tool.cpp",
			MAME_DIR .. "src/osd/modules/opengl/gl_shader_mgr.cpp",
			MAME_DIR .. "src/osd/modules/opengl/gl_shader_mgr.h",
			MAME_DIR .. "src/osd/modules/opengl/gl_shader_tool.h",
			MAME_DIR .. "src/osd/modules/opengl/osd_opengl.h",
		}
		defines {
			"USE_OPENGL=1",
		}
		if _OPTIONS["USE_DISPATCH_GL"]=="1" then
			defines {
				"USE_DISPATCH_GL=1",
			}
		end
	end

	defines {
		"__STDC_LIMIT_MACROS",
		"__STDC_FORMAT_MACROS",
		"__STDC_CONSTANT_MACROS",
	}

	files {
		MAME_DIR .. "src/osd/modules/render/drawbgfx.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfxutil.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfxutil.h",
		MAME_DIR .. "src/osd/modules/render/binpacker.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/blendreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/blendreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/chain.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/chain.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainentry.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainentry.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainentryreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainentryreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainmanager.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainmanager.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/chainreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/clear.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/clear.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/clearreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/clearreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/cullreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/cullreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/depthreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/depthreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/effect.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/effect.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/effectmanager.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/effectmanager.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/effectreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/effectreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/entryuniformreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/entryuniformreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/inputpair.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/inputpair.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/frameparameter.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/frameparameter.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/timeparameter.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/timeparameter.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/paramreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/paramreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/paramuniform.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/paramuniform.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/paramuniformreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/paramuniformreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/shadermanager.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/shadermanager.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/slider.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/slider.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/sliderreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/sliderreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/slideruniform.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/slideruniform.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/slideruniformreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/slideruniformreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/statereader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/statereader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/suppressor.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/suppressor.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/suppressorreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/suppressorreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/target.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/target.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/targetreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/targetreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/targetmanager.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/targetmanager.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/texture.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/texture.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/texturehandleprovider.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/texturemanager.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/texturemanager.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/uniform.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/uniform.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/uniformreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/uniformreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/valueuniform.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/valueuniform.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/valueuniformreader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/valueuniformreader.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/view.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/view.h",
		MAME_DIR .. "src/osd/modules/render/bgfx/writereader.cpp",
		MAME_DIR .. "src/osd/modules/render/bgfx/writereader.h",
	}
	includedirs {
		MAME_DIR .. "3rdparty/bgfx/examples/common",
		MAME_DIR .. "3rdparty/bgfx/include",
		MAME_DIR .. "3rdparty/bgfx/3rdparty",
		MAME_DIR .. "3rdparty/bgfx/3rdparty/khronos",
		MAME_DIR .. "3rdparty/bx/include",
		ext_includedir("rapidjson")
	}

	if _OPTIONS["NO_USE_PORTAUDIO"]=="1" then
		defines {
			"NO_USE_PORTAUDIO",
		}
	else
		includedirs {
			ext_includedir("portaudio"),
		}
	end

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

	if _OPTIONS["NO_OPENGL"]~="1" then
		if _OPTIONS["targetos"]=="macosx" then
			links {
				"OpenGL.framework",
			}
		elseif _OPTIONS["USE_DISPATCH_GL"]~="1" then
			if _OPTIONS["targetos"]=="windows" then
				links {
					"opengl32",
				}
			else
				links {
					"GL",
				}
			end
		end
	end

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
			"CoreAudio.framework",
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
	trigger = "NO_OPENGL",
	description = "Disable use of OpenGL",
	allowed = {
		{ "0",  "Enable OpenGL"  },
		{ "1",  "Disable OpenGL" },
	},
}

newoption {
	trigger = "USE_DISPATCH_GL",
	description = "Use GL-dispatching",
	allowed = {
		{ "0",  "Link to OpenGL library"  },
		{ "1",  "Use GL-dispatching"      },
	},
}

if not _OPTIONS["USE_DISPATCH_GL"] then
	_OPTIONS["USE_DISPATCH_GL"] = "0"
end

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
	trigger = "NO_USE_PORTAUDIO",
	description = "Disable PortAudio interface",
	allowed = {
		{ "0",  "Enable PortAudio"  },
		{ "1",  "Disable PortAudio" },
	},
}

if not _OPTIONS["NO_USE_PORTAUDIO"] then
	if _OPTIONS["targetos"]=="windows" or _OPTIONS["targetos"]=="linux" or _OPTIONS["targetos"]=="macosx" then
		_OPTIONS["NO_USE_PORTAUDIO"] = "0"
	else
		_OPTIONS["NO_USE_PORTAUDIO"] = "1"
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
