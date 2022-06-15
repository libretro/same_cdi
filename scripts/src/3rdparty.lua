-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   3rdparty.lua
--
--   Library objects for all 3rdparty sources
--
---------------------------------------------------------------------------

--------------------------------------------------
-- expat library objects
--------------------------------------------------

if not _OPTIONS["with-system-expat"] then
project "expat"
	uuid "f4cd40b1-c37c-452d-9785-640f26f0bf54"
	kind "StaticLib"

	-- fake out the enough of expat_config.h to get by
	-- could possibly add more defines here for specific targets
	defines {
		"HAVE_MEMMOVE",
		"HAVE_STDINT_H",
		"HAVE_STDLIB_H",
		"HAVE_STRING_H",
		"PACKAGE=\"expat\"",
		"PACKAGE_BUGREPORT=\"expat-bugs@libexpat.org\"",
		"PACKAGE_NAME=\"expat\"",
		"PACKAGE_STRING=\"expat 2.2.10\"",
		"PACKAGE_TARNAME=\"expat\"",
		"PACKAGE_URL=\"\"",
		"PACKAGE_VERSION=\"2.2.10\"",
		"STDC_HEADERS",
		"VERSION=\"2.2.10\"",
		"XML_CONTEXT_BYTES=1024",
		"XML_DTD",
		"XML_NS",
	}
if _OPTIONS["BIGENDIAN"]=="1" then
	defines {
		"BYTEORDER=4321",
		"WORDS_BIGENDIAN",
	}
else
	defines {
		"BYTEORDER=1234",
	}
end
if _OPTIONS["targetos"]=="macosx" or _OPTIONS["targetos"]=="freebsd" then
	defines {
		"HAVE_ARC4RANDOM",
	}
end
if BASE_TARGETOS=="unix" then
	defines {
		"HAVE_DLFCN_H",
		"HAVE_FCNTL_H",
		"HAVE_MMAP",
		"HAVE_SYS_STAT_H",
		"HAVE_SYS_TYPES_H",
		"HAVE_UNISTD_H",
		"XML_DEV_URANDOM",
	}
end

	configuration { "vs*" }
		buildoptions {
			"/wd4100", -- warning C4100: 'xxx' : unreferenced formal parameter
			"/wd4127", -- warning C4127: conditional expression is constant
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
			"/wd4456", -- warning C4456: declaration of 'xxx' hides previous local declaration
		}
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd111",              -- remark #111: statement is unreachable
			"/Qwd1879",             -- warning #1879: unimplemented pragma ignored
			"/Qwd2557",             -- remark #2557: comparison between signed and unsigned operands
			"/Qwd869",              -- remark #869: parameter "xxx" was never referenced
		}
end
	configuration { }

	files {
		MAME_DIR .. "3rdparty/expat/lib/xmlparse.c",
		MAME_DIR .. "3rdparty/expat/lib/xmlrole.c",
		MAME_DIR .. "3rdparty/expat/lib/xmltok.c",
	}
else
links {
	ext_lib("expat"),
}
end


--------------------------------------------------
-- zlib library objects
--------------------------------------------------

if not _OPTIONS["with-system-zlib"] then
project "zlib"
	uuid "3d78bd2a-2bd0-4449-8087-42ddfaef7ec9"
	kind "StaticLib"

	local version = str_to_version(_OPTIONS["gcc_version"])
	if _OPTIONS["gcc"]~=nil and ((string.find(_OPTIONS["gcc"], "clang") or string.find(_OPTIONS["gcc"], "asmjs") or string.find(_OPTIONS["gcc"], "android"))) then
		configuration { "gmake or ninja" }
		if (version >= 30700) then
			buildoptions {
				"-Wno-shift-negative-value",
			}
		end
	end

	configuration { "vs*" }
		buildoptions {
			"/wd4131", -- warning C4131: 'xxx' : uses old-style declarator
			"/wd4127", -- warning C4127: conditional expression is constant
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
		}
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd111",              -- remark #111: statement is unreachable
			"/Qwd280",              -- remark #280: selector expression is constant
		}
end
	configuration "Debug"
		defines {
			"verbose=-1",
		}

	configuration { "gmake or ninja" }
		buildoptions_c {
			"-Wno-strict-prototypes",
		}

	configuration { }
		defines {
			"ZLIB_CONST",
		}

	files {
		MAME_DIR .. "3rdparty/zlib/adler32.c",
		MAME_DIR .. "3rdparty/zlib/compress.c",
		MAME_DIR .. "3rdparty/zlib/crc32.c",
		MAME_DIR .. "3rdparty/zlib/deflate.c",
		MAME_DIR .. "3rdparty/zlib/inffast.c",
		MAME_DIR .. "3rdparty/zlib/inflate.c",
		MAME_DIR .. "3rdparty/zlib/infback.c",
		MAME_DIR .. "3rdparty/zlib/inftrees.c",
		MAME_DIR .. "3rdparty/zlib/trees.c",
		MAME_DIR .. "3rdparty/zlib/uncompr.c",
		MAME_DIR .. "3rdparty/zlib/zutil.c",
	}
else
links {
	ext_lib("zlib"),
}
end


--------------------------------------------------
-- SoftFloat library objects
--------------------------------------------------

project "softfloat"
	uuid "04fbf89e-4761-4cf2-8a12-64500cf0c5c5"
	kind "StaticLib"

	options {
		"ForceCPP",
	}

	includedirs {
		MAME_DIR .. "src/osd",
	}

	configuration { "gmake or ninja" }
		buildoptions_cpp {
			"-x c++",
		}

	configuration { "vs*" }
		buildoptions {
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
			"/wd4146", -- warning C4146: unary minus operator applied to unsigned type, result still unsigned
			"/wd4018", -- warning C4018: 'x' : signed/unsigned mismatch
		}
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd2557",             -- remark #2557: comparison between signed and unsigned operands
		}
end
	configuration { }

	files {
		MAME_DIR .. "3rdparty/softfloat/softfloat.c",
		MAME_DIR .. "3rdparty/softfloat/fsincos.c",
		MAME_DIR .. "3rdparty/softfloat/fpatan.c",
		MAME_DIR .. "3rdparty/softfloat/fyl2x.c",
	}


--------------------------------------------------
-- SoftFloat 3 library objects
--------------------------------------------------

project "softfloat3"
uuid "9c22fc90-53fd-11e8-b566-0800200c9a66"
kind "StaticLib"

options {
	"ForceCPP",
}

includedirs {
	MAME_DIR .. "src/osd",
	MAME_DIR .. "3rdparty/softfloat3/build/MAME",
	MAME_DIR .. "3rdparty/softfloat3/source",
	MAME_DIR .. "3rdparty/softfloat3/source/include",
	MAME_DIR .. "3rdparty/softfloat3/source/8086",
}

configuration { "gmake or ninja" }
buildoptions_cpp {
	"-x c++",
}
if _OPTIONS["gcc"]~=nil and not string.find(_OPTIONS["gcc"], "clang") then
	buildoptions_cpp {
		"-Wno-error=implicit-fallthrough",
	}
end

configuration { "vs*" }
buildoptions {
	"/wd4701", -- warning C4701: potentially uninitialized local variable 'xxx' used
	"/wd4703", -- warning C4703: potentially uninitialized local pointer variable 'xxx' used
}

configuration { }
defines {
	"SOFTFLOAT_ROUND_ODD",
	"INLINE_LEVEL=5",
	"SOFTFLOAT_FAST_DIV32TO16",
	"SOFTFLOAT_FAST_DIV64TO32",
	"SOFTFLOAT_FAST_INT64"
}

files {
	MAME_DIR .. "3rdparty/softfloat3/source/s_eq128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_le128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_lt128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shortShiftLeft128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shortShiftRight128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shortShiftRightJam64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shortShiftRightJam64Extra.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shortShiftRightJam128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shortShiftRightJam128Extra.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shiftRightJam32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shiftRightJam64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shiftRightJam64Extra.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shiftRightJam128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shiftRightJam128Extra.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_shiftRightJam256M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_countLeadingZeros8.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_countLeadingZeros16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_countLeadingZeros32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_countLeadingZeros64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_add128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_add256M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_sub128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_sub256M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mul64ByShifted32To128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mul64To128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mul128By32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mul128To256M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_approxRecip_1Ks.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_approxRecip32_1.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_approxRecipSqrt_1Ks.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_approxRecipSqrt32_1.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/softfloat_raiseFlags.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_commonNaNToF16UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_propagateNaNF16UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_f32UIToCommonNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_commonNaNToF32UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_propagateNaNF32UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_f64UIToCommonNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_commonNaNToF64UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_propagateNaNF64UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/extF80M_isSignalingNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_extF80UIToCommonNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_commonNaNToExtF80UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_propagateNaNExtF80UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/f128M_isSignalingNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_f128UIToCommonNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_commonNaNToF128UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/8086/s_propagateNaNF128UI.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundToUI32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundToUI64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundToI32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundToI64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normSubnormalF16Sig.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundPackToF16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normRoundPackToF16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_addMagsF16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_subMagsF16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mulAddF16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normSubnormalF32Sig.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundPackToF32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normRoundPackToF32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_addMagsF32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_subMagsF32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mulAddF32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normSubnormalF64Sig.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundPackToF64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normRoundPackToF64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_addMagsF64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_subMagsF64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mulAddF64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normSubnormalExtF80Sig.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundPackToExtF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normRoundPackToExtF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_addMagsExtF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_subMagsExtF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normSubnormalF128Sig.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_roundPackToF128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_normRoundPackToF128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_addMagsF128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_subMagsF128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/s_mulAddF128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/softfloat_state.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui32_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui32_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui32_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui32_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui32_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui32_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui32_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui64_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui64_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui64_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui64_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui64_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui64_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/ui64_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i32_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i32_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i32_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i32_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i32_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i32_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i32_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i64_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i64_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i64_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i64_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i64_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i64_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/i64_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_ui32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_ui64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_i32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_i64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_ui32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_ui64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_i32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_i64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_roundToInt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_add.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_sub.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_mul.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_mulAdd.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_div.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_rem.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_sqrt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_eq.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_le.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_lt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_eq_signaling.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_le_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_lt_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f16_isSignalingNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_ui32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_ui64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_i32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_i64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_ui32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_ui64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_i32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_i64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_roundToInt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_add.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_sub.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_mul.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_mulAdd.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_div.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_rem.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_sqrt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_eq.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_le.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_lt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_eq_signaling.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_le_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_lt_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f32_isSignalingNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_ui32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_ui64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_i32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_i64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_ui32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_ui64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_i32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_i64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_roundToInt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_add.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_sub.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_mul.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_mulAdd.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_div.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_rem.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_sqrt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_eq.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_le.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_lt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_eq_signaling.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_le_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_lt_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f64_isSignalingNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_ui32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_ui64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_i32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_i64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_ui32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_ui64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_i32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_i64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_to_f128.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_roundToInt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_add.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_sub.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_mul.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_div.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_rem.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_sqrt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_eq.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_le.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_lt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_eq_signaling.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_le_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_lt_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80_isSignalingNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_ui32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_ui64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_i32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_i64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_ui32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_ui64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_i32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_i64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_to_f128M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_roundToInt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_add.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_sub.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_mul.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_div.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_rem.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_sqrt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_eq.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_le.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_lt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_eq_signaling.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_le_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/extF80M_lt_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_ui32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_ui64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_i32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_i64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_ui32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_ui64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_i32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_i64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_extF80.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_roundToInt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_add.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_sub.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_mul.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_mulAdd.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_div.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_rem.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_sqrt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_eq.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_le.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_lt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_eq_signaling.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_le_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_lt_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128_isSignalingNaN.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_ui32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_ui64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_i32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_i64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_ui32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_ui64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_i32_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_i64_r_minMag.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_f16.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_f32.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_extF80M.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_to_f64.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_roundToInt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_add.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_sub.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_mul.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_mulAdd.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_div.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_rem.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_sqrt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_eq.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_le.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_lt.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_eq_signaling.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_le_quiet.c",
	MAME_DIR .. "3rdparty/softfloat3/source/f128M_lt_quiet.c",
}


-------------------------------------------------
-- libJPEG library objects
--------------------------------------------------

if not _OPTIONS["with-system-jpeg"] then
project "jpeg"
	uuid "447c6800-dcfd-4c48-b72a-a8223bb409ca"
	kind "StaticLib"

	configuration { "vs*" }
		buildoptions {
			"/wd4100", -- warning C4100: 'xxx' : unreferenced formal parameter
			"/wd4127", -- warning C4127: conditional expression is constant
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
		}
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd869",              -- remark #869: parameter "xxx" was never referenced
		}
end

	configuration { }

	files {
		MAME_DIR .. "3rdparty/libjpeg/jaricom.c",
		MAME_DIR .. "3rdparty/libjpeg/jcapimin.c",
		MAME_DIR .. "3rdparty/libjpeg/jcapistd.c",
		MAME_DIR .. "3rdparty/libjpeg/jcarith.c",
		MAME_DIR .. "3rdparty/libjpeg/jccoefct.c",
		MAME_DIR .. "3rdparty/libjpeg/jccolor.c",
		MAME_DIR .. "3rdparty/libjpeg/jcdctmgr.c",
		MAME_DIR .. "3rdparty/libjpeg/jchuff.c",
		MAME_DIR .. "3rdparty/libjpeg/jcinit.c",
		MAME_DIR .. "3rdparty/libjpeg/jcmainct.c",
		MAME_DIR .. "3rdparty/libjpeg/jcmarker.c",
		MAME_DIR .. "3rdparty/libjpeg/jcmaster.c",
		MAME_DIR .. "3rdparty/libjpeg/jcomapi.c",
		MAME_DIR .. "3rdparty/libjpeg/jcparam.c",
		MAME_DIR .. "3rdparty/libjpeg/jcprepct.c",
		MAME_DIR .. "3rdparty/libjpeg/jcsample.c",
		MAME_DIR .. "3rdparty/libjpeg/jctrans.c",
		MAME_DIR .. "3rdparty/libjpeg/jdapimin.c",
		MAME_DIR .. "3rdparty/libjpeg/jdapistd.c",
		MAME_DIR .. "3rdparty/libjpeg/jdarith.c",
		MAME_DIR .. "3rdparty/libjpeg/jdatadst.c",
		MAME_DIR .. "3rdparty/libjpeg/jdatasrc.c",
		MAME_DIR .. "3rdparty/libjpeg/jdcoefct.c",
		MAME_DIR .. "3rdparty/libjpeg/jdcolor.c",
		MAME_DIR .. "3rdparty/libjpeg/jddctmgr.c",
		MAME_DIR .. "3rdparty/libjpeg/jdhuff.c",
		MAME_DIR .. "3rdparty/libjpeg/jdinput.c",
		MAME_DIR .. "3rdparty/libjpeg/jdmainct.c",
		MAME_DIR .. "3rdparty/libjpeg/jdmarker.c",
		MAME_DIR .. "3rdparty/libjpeg/jdmaster.c",
		MAME_DIR .. "3rdparty/libjpeg/jdmerge.c",
		MAME_DIR .. "3rdparty/libjpeg/jdpostct.c",
		MAME_DIR .. "3rdparty/libjpeg/jdsample.c",
		MAME_DIR .. "3rdparty/libjpeg/jdtrans.c",
		MAME_DIR .. "3rdparty/libjpeg/jerror.c",
		MAME_DIR .. "3rdparty/libjpeg/jfdctflt.c",
		MAME_DIR .. "3rdparty/libjpeg/jfdctfst.c",
		MAME_DIR .. "3rdparty/libjpeg/jfdctint.c",
		MAME_DIR .. "3rdparty/libjpeg/jidctflt.c",
		MAME_DIR .. "3rdparty/libjpeg/jidctfst.c",
		MAME_DIR .. "3rdparty/libjpeg/jidctint.c",
		MAME_DIR .. "3rdparty/libjpeg/jquant1.c",
		MAME_DIR .. "3rdparty/libjpeg/jquant2.c",
		MAME_DIR .. "3rdparty/libjpeg/jutils.c",
		MAME_DIR .. "3rdparty/libjpeg/jmemmgr.c",
		MAME_DIR .. "3rdparty/libjpeg/jmemansi.c",
	}
else
links {
	ext_lib("jpeg"),
}
end


--------------------------------------------------
-- libflac library objects
--------------------------------------------------

if not _OPTIONS["with-system-flac"] then
project "flac"
	uuid "b6fc19e8-073a-4541-bb7b-d24b548d424a"
	kind "StaticLib"

	configuration { "vs*" }
		buildoptions {
			"/wd4127", -- warning C4127: conditional expression is constant
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
			"/wd4100", -- warning C4100: 'xxx' : unreferenced formal parameter
			"/wd4456", -- warning C4456: declaration of 'xxx' hides previous local declaration
			"/wd4702", -- warning C4702: unreachable code
		}
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd111",              -- remark #111: statement is unreachable
			"/Qwd177",              -- remark #177: function "xxx" was declared but never referenced
			"/Qwd181",              -- remark #181: argument of type "UINT32={unsigned int}" is incompatible with format "%d", expecting argument of type "int"
			"/Qwd188",              -- error #188: enumerated type mixed with another type
			"/Qwd869",              -- remark #869: parameter "xxx" was never referenced
		}
end

	configuration { "mingw-clang" }
		buildoptions {
			"-include stdint.h"
		}

	configuration { }
		defines {
			"WORDS_BIGENDIAN=0",
			"FLAC__NO_ASM",
			"_LARGEFILE_SOURCE",
			"_FILE_OFFSET_BITS=64",
			"FLAC__HAS_OGG=0",
			"HAVE_CONFIG_H=1",
		}

	configuration { "gmake or ninja" }
		buildoptions_c {
			"-Wno-unused-function",
			"-O0",
		}
	if _OPTIONS["gcc"]~=nil and (string.find(_OPTIONS["gcc"], "clang") or string.find(_OPTIONS["gcc"], "android")) then
		buildoptions {
			"-Wno-enum-conversion",
		}
		if _OPTIONS["targetos"]=="macosx" then
			buildoptions_c {
				"-Wno-unknown-attributes",
			}
		end
	end
	configuration { }

	includedirs {
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/include",
		MAME_DIR .. "3rdparty/libflac/include",
	}

	files {
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/bitmath.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/bitreader.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/bitwriter.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/cpu.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/crc.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/fixed.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/float.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/format.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/lpc.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/md5.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/memory.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/stream_decoder.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/stream_encoder.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/stream_encoder_framing.c",
		MAME_DIR .. "3rdparty/libflac/src/libFLAC/window.c",
	}
else
links {
	ext_lib("flac"),
}
end


--------------------------------------------------
-- lib7z library objects
--------------------------------------------------

project "7z"
	uuid "ad573d62-e76a-4b11-ae34-5110a6789a42"
	kind "StaticLib"

	configuration { "gmake or ninja" }
		buildoptions_c {
			"-Wno-strict-prototypes",
			"-Wno-undef",
		}
if _OPTIONS["gcc"]~=nil and string.find(_OPTIONS["gcc"], "clang") and str_to_version(_OPTIONS["gcc_version"]) >= 100000 then
		buildoptions_c {
			"-Wno-misleading-indentation",
		}
end

	configuration { "asmjs" }
		buildoptions {
			"-Wno-misleading-indentation",
		}

	configuration { "mingw*" }
		buildoptions_c {
			"-Wno-strict-prototypes",
		}

	configuration { "vs*" }
		buildoptions {
			"/wd4100", -- warning C4100: 'xxx' : unreferenced formal parameter
			"/wd4456", -- warning C4456: declaration of 'xxx' hides previous local declaration
			"/wd4457", -- warning C4457: declaration of 'xxx' hides function parameter
		}
if _OPTIONS["vs"]=="clangcl" then
		buildoptions {
			"-Wno-misleading-indentation",
		}
end
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd869",              -- remark #869: parameter "xxx" was never referenced
		}
end
	configuration { }
		defines {
			"_7ZIP_PPMD_SUPPPORT",
			"_7ZIP_ST",
		}

	files {
			MAME_DIR .. "3rdparty/lzma/C/7zAlloc.c",
			MAME_DIR .. "3rdparty/lzma/C/7zArcIn.c",
			MAME_DIR .. "3rdparty/lzma/C/7zBuf.c",
			MAME_DIR .. "3rdparty/lzma/C/7zBuf2.c",
			MAME_DIR .. "3rdparty/lzma/C/7zCrc.c",
			MAME_DIR .. "3rdparty/lzma/C/7zCrcOpt.c",
			MAME_DIR .. "3rdparty/lzma/C/7zDec.c",
			MAME_DIR .. "3rdparty/lzma/C/7zFile.c",
			MAME_DIR .. "3rdparty/lzma/C/7zStream.c",
			MAME_DIR .. "3rdparty/lzma/C/Aes.c",
			MAME_DIR .. "3rdparty/lzma/C/AesOpt.c",
			MAME_DIR .. "3rdparty/lzma/C/Alloc.c",
			MAME_DIR .. "3rdparty/lzma/C/Bcj2.c",
			-- MAME_DIR .. "3rdparty/lzma/C/Bcj2Enc.c",
			MAME_DIR .. "3rdparty/lzma/C/Bra.c",
			MAME_DIR .. "3rdparty/lzma/C/Bra86.c",
			MAME_DIR .. "3rdparty/lzma/C/BraIA64.c",
			MAME_DIR .. "3rdparty/lzma/C/CpuArch.c",
			MAME_DIR .. "3rdparty/lzma/C/Delta.c",
			-- MAME_DIR .. "3rdparty/lzma/C/DllSecur.c",
			MAME_DIR .. "3rdparty/lzma/C/LzFind.c",
			-- MAME_DIR .. "3rdparty/lzma/C/LzFindMt.c",
			MAME_DIR .. "3rdparty/lzma/C/Lzma2Dec.c",
			MAME_DIR .. "3rdparty/lzma/C/Lzma2Enc.c",
			MAME_DIR .. "3rdparty/lzma/C/Lzma86Dec.c",
			MAME_DIR .. "3rdparty/lzma/C/Lzma86Enc.c",
			MAME_DIR .. "3rdparty/lzma/C/LzmaDec.c",
			MAME_DIR .. "3rdparty/lzma/C/LzmaEnc.c",
			-- MAME_DIR .. "3rdparty/lzma/C/LzmaLib.c",
			-- MAME_DIR .. "3rdparty/lzma/C/MtCoder.c",
			MAME_DIR .. "3rdparty/lzma/C/Ppmd7.c",
			MAME_DIR .. "3rdparty/lzma/C/Ppmd7Dec.c",
			MAME_DIR .. "3rdparty/lzma/C/Ppmd7Enc.c",
			MAME_DIR .. "3rdparty/lzma/C/Sha256.c",
			MAME_DIR .. "3rdparty/lzma/C/Sort.c",
			-- MAME_DIR .. "3rdparty/lzma/C/Threads.c",
			-- MAME_DIR .. "3rdparty/lzma/C/Xz.c",
			-- MAME_DIR .. "3rdparty/lzma/C/XzCrc64.c",
			-- MAME_DIR .. "3rdparty/lzma/C/XzCrc64Opt.c",
			-- MAME_DIR .. "3rdparty/lzma/C/XzDec.c",
			-- MAME_DIR .. "3rdparty/lzma/C/XzEnc.c",
			-- MAME_DIR .. "3rdparty/lzma/C/XzIn.c",
		}


--------------------------------------------------
-- LUA library objects
--------------------------------------------------
if (STANDALONE~=true) then

if not _OPTIONS["with-system-lua"] then
project "lua"
	uuid "d9e2eed1-f1ab-4737-a6ac-863700b1a5a9"
	kind "StaticLib"

	-- uncomment the options below to
	-- compile using c++. Do the same
	-- in lualibs.
	-- In addition comment out the "extern "C""
	-- in lua.hpp and do the same in luaengine.c line 47
	--options {
	--  "ForceCPP",
	--}

	configuration { "gmake or ninja" }
		buildoptions_c {
			"-Wno-bad-function-cast"
		}

	configuration { "vs*" }
		buildoptions {
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
			"/wd4702", -- warning C4702: unreachable code
			"/wd4310", -- warning C4310: cast truncates constant value
		}
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd592", -- error #592: variable "xxx" is used before its value is set
		}
end

	configuration { }
		defines {
			"LUA_COMPAT_ALL",
			"LUA_COMPAT_5_1",
			"LUA_COMPAT_5_2",
		}
	if not (_OPTIONS["targetos"]=="windows") and not (_OPTIONS["targetos"]=="asmjs") then
		defines {
			"LUA_USE_POSIX",
		}
	end

	configuration { }

	includedirs {
		MAME_DIR .. "3rdparty",
	}

	files {
		MAME_DIR .. "3rdparty/lua/src/lapi.c",
		MAME_DIR .. "3rdparty/lua/src/lcode.c",
		MAME_DIR .. "3rdparty/lua/src/lctype.c",
		MAME_DIR .. "3rdparty/lua/src/ldebug.c",
		MAME_DIR .. "3rdparty/lua/src/ldo.c",
		MAME_DIR .. "3rdparty/lua/src/ldump.c",
		MAME_DIR .. "3rdparty/lua/src/lfunc.c",
		MAME_DIR .. "3rdparty/lua/src/lgc.c",
		MAME_DIR .. "3rdparty/lua/src/llex.c",
		MAME_DIR .. "3rdparty/lua/src/lmem.c",
		MAME_DIR .. "3rdparty/lua/src/lobject.c",
		MAME_DIR .. "3rdparty/lua/src/lopcodes.c",
		MAME_DIR .. "3rdparty/lua/src/lparser.c",
		MAME_DIR .. "3rdparty/lua/src/lstate.c",
		MAME_DIR .. "3rdparty/lua/src/lstring.c",
		MAME_DIR .. "3rdparty/lua/src/ltable.c",
		MAME_DIR .. "3rdparty/lua/src/ltm.c",
		MAME_DIR .. "3rdparty/lua/src/lundump.c",
		MAME_DIR .. "3rdparty/lua/src/lvm.c",
		MAME_DIR .. "3rdparty/lua/src/lzio.c",
		MAME_DIR .. "3rdparty/lua/src/lauxlib.c",
		MAME_DIR .. "3rdparty/lua/src/lbaselib.c",
		MAME_DIR .. "3rdparty/lua/src/lbitlib.c",
		MAME_DIR .. "3rdparty/lua/src/lcorolib.c",
		MAME_DIR .. "3rdparty/lua/src/ldblib.c",
		MAME_DIR .. "3rdparty/lua/src/liolib.c",
		MAME_DIR .. "3rdparty/lua/src/lmathlib.c",
		MAME_DIR .. "3rdparty/lua/src/loslib.c",
		MAME_DIR .. "3rdparty/lua/src/lstrlib.c",
		MAME_DIR .. "3rdparty/lua/src/ltablib.c",
		MAME_DIR .. "3rdparty/lua/src/loadlib.c",
		MAME_DIR .. "3rdparty/lua/src/linit.c",
		MAME_DIR .. "3rdparty/lua/src/lutf8lib.c",
	}
else
links {
	ext_lib("lua"),
}
end


--------------------------------------------------
-- small lua library objects
--------------------------------------------------

project "lualibs"
	uuid "1d84edab-94cf-48fb-83ee-b75bc697660e"
	kind "StaticLib"

	configuration { "vs*" }
		buildoptions {
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
			"/wd4055", -- warning C4055: 'type cast': from data pointer 'void *' to function pointer 'xxx'
			"/wd4152", -- warning C4152: nonstandard extension, function/data pointer conversion in expression
			"/wd4130", -- warning C4130: '==': logical operation on address of string constant
		}

	configuration { }
		defines {
			"LUA_COMPAT_ALL",
		}

	includedirs {
		MAME_DIR .. "3rdparty",
		MAME_DIR .. "3rdparty/linenoise",
	}

	includedirs {
		ext_includedir("lua"),
		ext_includedir("zlib"),
		ext_includedir("sqlite3"),
	}

	configuration {}

	files {
		MAME_DIR .. "3rdparty/lsqlite3/lsqlite3.c",
		MAME_DIR .. "3rdparty/lua-zlib/lua_zlib.c",
		MAME_DIR .. "3rdparty/luafilesystem/src/lfs.c",
		MAME_DIR .. "3rdparty/lua-linenoise/linenoise.c",
	}

--------------------------------------------------
-- SQLite3 library objects
--------------------------------------------------

if not _OPTIONS["with-system-sqlite3"] then
project "sqlite3"
	uuid "5cb3d495-57ed-461c-81e5-80dc0857517d"
	kind "StaticLib"

	configuration { "gmake" }
		buildoptions_c {
			"-Wno-bad-function-cast",
			"-Wno-discarded-qualifiers",
			"-Wno-undef",
			"-Wno-unused-but-set-variable",
		}
if _OPTIONS["gcc"]~=nil and ((string.find(_OPTIONS["gcc"], "clang") or string.find(_OPTIONS["gcc"], "asmjs") or string.find(_OPTIONS["gcc"], "android"))) then
		buildoptions_c {
			"-Wno-incompatible-pointer-types-discards-qualifiers",
		}
end
	configuration { "vs*" }
if _OPTIONS["vs"]=="clangcl" then
		buildoptions {
			"-Wno-implicit-int-float-conversion",
		}
end

	configuration { }

	files {
		MAME_DIR .. "3rdparty/sqlite3/sqlite3.c",
	}
else
links {
	ext_lib("sqlite3"),
}
end

end


--------------------------------------------------
-- portmidi library objects
--------------------------------------------------

if _OPTIONS["NO_USE_MIDI"]~="1" then
if not _OPTIONS["with-system-portmidi"] then
project "portmidi"
	uuid "587f2da6-3274-4a65-86a2-f13ea315bb98"
	kind "StaticLib"

	includedirs {
		MAME_DIR .. "3rdparty/portmidi/pm_common",
		MAME_DIR .. "3rdparty/portmidi/porttime",
	}

	configuration { "vs*" }
		buildoptions {
			"/wd4100", -- warning C4100: 'xxx' : unreferenced formal parameter
			"/wd4127", -- warning C4127: conditional expression is constant
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
			"/wd4456", -- warning C4456: declaration of 'xxx' hides previous local declaration
			"/wd4706", -- warning C4706: assignment within conditional expression
		}
if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd188",              -- error #188: enumerated type mixed with another type
			"/Qwd344",              -- remark #344: typedef name has already been declared (with same type)
			"/Qwd869",              -- remark #869: parameter "xxx" was never referenced
			"/Qwd2557",             -- remark #2557: comparison between signed and unsigned operands
		}
end

	configuration { "linux*" }
		defines {
			"PMALSA=1",
		}

	configuration { }

	files {
		MAME_DIR .. "3rdparty/portmidi/pm_common/portmidi.c",
		MAME_DIR .. "3rdparty/portmidi/pm_common/pmutil.c",
	}

	if _OPTIONS["targetos"]=="windows" then
		files {
			MAME_DIR .. "3rdparty/portmidi/porttime/ptwinmm.c",
			MAME_DIR .. "3rdparty/portmidi/pm_win/pmwin.c",
			MAME_DIR .. "3rdparty/portmidi/pm_win/pmwinmm.c",
			MAME_DIR .. "3rdparty/portmidi/porttime/ptwinmm.c",
		}
	end

	if _OPTIONS["targetos"]=="linux" then
		files {
			MAME_DIR .. "3rdparty/portmidi/pm_linux/pmlinux.c",
			MAME_DIR .. "3rdparty/portmidi/pm_linux/pmlinuxalsa.c",
			MAME_DIR .. "3rdparty/portmidi/pm_linux/finddefault.c",
			MAME_DIR .. "3rdparty/portmidi/porttime/ptlinux.c",
		}
	end
	if _OPTIONS["targetos"]=="netbsd" then
		files {
			MAME_DIR .. "3rdparty/portmidi/pm_linux/pmlinux.c",
			MAME_DIR .. "3rdparty/portmidi/pm_linux/finddefault.c",
			MAME_DIR .. "3rdparty/portmidi/porttime/ptlinux.c",
		}
	end
	if _OPTIONS["targetos"]=="macosx" then
		files {
			MAME_DIR .. "3rdparty/portmidi/pm_mac/pmmac.c",
			MAME_DIR .. "3rdparty/portmidi/pm_mac/pmmacosxcm.c",
			MAME_DIR .. "3rdparty/portmidi/pm_mac/finddefault.c",
			MAME_DIR .. "3rdparty/portmidi/pm_mac/readbinaryplist.c",
			MAME_DIR .. "3rdparty/portmidi/pm_mac/osxsupport.m",
			MAME_DIR .. "3rdparty/portmidi/porttime/ptmacosx_mach.c",
		}
	end
else
links {
	ext_lib("portmidi"),
}
end
end

--------------------------------------------------
-- PortAudio library objects
--------------------------------------------------

if _OPTIONS["NO_USE_PORTAUDIO"]~="1" then
if not _OPTIONS["with-system-portaudio"] then
project "portaudio"
	uuid "0755c5f5-eccf-47f3-98a9-df67018a94d4"
	kind "StaticLib"

	configuration { "vs*" }
		buildoptions {
			"/wd4245", -- warning C4245: 'conversion' : conversion from 'type1' to 'type2', signed/unsigned mismatch
			"/wd4244", -- warning C4244: 'argument' : conversion from 'xxx' to 'xxx', possible loss of data
			"/wd4100", -- warning C4100: 'xxx' : unreferenced formal parameter
			"/wd4389", -- warning C4389: 'operator' : signed/unsigned mismatch
			"/wd4189", -- warning C4189: 'xxx' : local variable is initialized but not referenced
			"/wd4127", -- warning C4127: conditional expression is constant
			"/wd4456", -- warning C4456: declaration of 'xxx' hides previous local declaration
			"/wd4312", -- warning C4312: 'type cast': conversion from 'UINT' to 'HWAVEIN' of greater size
		}
	if _OPTIONS["vs"]=="intel-15" then
		buildoptions {
			"/Qwd869",              -- remark #869: parameter "xxx" was never referenced
			"/Qwd1478",             -- warning #1478: function "xxx" (declared at line yyy of "zzz") was declared deprecated
			"/Qwd2544",             -- message #2544: empty dependent statement in if-statement
			"/Qwd1879",             -- warning #1879: unimplemented pragma ignored
		}
	end

	configuration { "gmake or ninja" }
		buildoptions_c {
			"-Wno-bad-function-cast",
			"-Wno-missing-braces",
			"-Wno-strict-prototypes",
			"-Wno-undef",
			"-Wno-unknown-pragmas",
			"-Wno-unused-function",
			"-Wno-unused-value",
			"-Wno-unused-variable",
		}

	local version = str_to_version(_OPTIONS["gcc_version"])
	if (_OPTIONS["gcc"]~=nil) then
		if string.find(_OPTIONS["gcc"], "clang") or string.find(_OPTIONS["gcc"], "android") then
			buildoptions_c {
				"-Wno-unknown-warning-option",
				"-Wno-absolute-value",
				"-Wno-unused-but-set-variable",
				"-Wno-maybe-uninitialized",
				"-Wno-sometimes-uninitialized",
			}
		else
			buildoptions_c {
				"-Wno-maybe-uninitialized",
				"-Wno-sometimes-uninitialized",
				"-Wno-unused-but-set-variable",
				"-Wno-incompatible-pointer-types-discards-qualifiers",
				"-w",
			}
		end
		if string.find(_OPTIONS["gcc"], "clang") and version >= 100000 then
			buildoptions_c {
				"-Wno-misleading-indentation",
			}
		end
	end
	configuration { "vs*" }
		buildoptions {
			"/wd4204", -- warning C4204: nonstandard extension used : non-constant aggregate initializer
			"/wd4701", -- warning C4701: potentially uninitialized local variable 'xxx' used
			"/wd4057", -- warning C4057: 'function': 'xxx' differs in indirection to slightly different base types from 'xxx'
		}

	configuration { }

	includedirs {
		MAME_DIR .. "3rdparty/portaudio/include",
		MAME_DIR .. "3rdparty/portaudio/src/common",
	}

	files {
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_allocation.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_converters.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_cpuload.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_dither.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_debugprint.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_front.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_process.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_stream.c",
		MAME_DIR .. "3rdparty/portaudio/src/common/pa_trace.c",
		MAME_DIR .. "3rdparty/portaudio/src/hostapi/skeleton/pa_hostapi_skeleton.c",
	}

	if _OPTIONS["targetos"]=="windows" then
		defines {
			"PA_USE_DS=1",
			"PA_USE_WASAPI=1",
			"PA_USE_WDMKS=1",
			"PA_USE_WMME=1",
		}
		includedirs {
			MAME_DIR .. "3rdparty/portaudio/src/os/win",
		}

		configuration { "mingw*" }
		includedirs {
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/wasapi/mingw-include",
		}

		configuration { }
		files {
			MAME_DIR .. "3rdparty/portaudio/src/os/win/pa_win_util.c",
			MAME_DIR .. "3rdparty/portaudio/src/os/win/pa_win_waveformat.c",
			MAME_DIR .. "3rdparty/portaudio/src/os/win/pa_win_hostapis.c",
			MAME_DIR .. "3rdparty/portaudio/src/os/win/pa_win_coinitialize.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/dsound/pa_win_ds.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/dsound/pa_win_ds_dynlink.c",
			MAME_DIR .. "3rdparty/portaudio/src/os/win/pa_win_hostapis.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/wasapi/pa_win_wasapi.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/wdmks/pa_win_wdmks.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/wmme/pa_win_wmme.c",
			MAME_DIR .. "3rdparty/portaudio/src/common/pa_ringbuffer.c",
		}

	end
	if _OPTIONS["targetos"]=="linux" then
		defines {
			"PA_USE_ALSA=1",
			"PA_USE_OSS=1",
			"HAVE_LINUX_SOUNDCARD_H",
		}
		includedirs {
			MAME_DIR .. "3rdparty/portaudio/src/os/unix",
		}
		files {
			MAME_DIR .. "3rdparty/portaudio/src/os/unix/pa_unix_hostapis.c",
			MAME_DIR .. "3rdparty/portaudio/src/os/unix/pa_unix_util.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/alsa/pa_linux_alsa.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/oss/pa_unix_oss.c",
		}
	end
	if _OPTIONS["targetos"]=="macosx" then
		defines {
			"PA_USE_COREAUDIO=1",
		}
		includedirs {
			MAME_DIR .. "3rdparty/portaudio/src/os/unix",
		}
		files {
			MAME_DIR .. "3rdparty/portaudio/src/os/unix/pa_unix_hostapis.c",
			MAME_DIR .. "3rdparty/portaudio/src/os/unix/pa_unix_util.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/coreaudio/pa_mac_core.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/coreaudio/pa_mac_core_utilities.c",
			MAME_DIR .. "3rdparty/portaudio/src/hostapi/coreaudio/pa_mac_core_blocking.c",
			MAME_DIR .. "3rdparty/portaudio/src/common/pa_ringbuffer.c",
		}
	end

else
links {
	ext_lib("portaudio"),
}
end
end

--------------------------------------------------
-- linenoise library
--------------------------------------------------

project "linenoise"
	uuid "7320ffc8-2748-4add-8864-ae29b72a8511"
	kind (LIBTYPE)

	addprojectflags()

	configuration { "vs*" }
		buildoptions {
			"/wd4701", -- warning C4701: potentially uninitialized local variable 'xxx' used
		}

	configuration { }

	includedirs {
		MAME_DIR .. "3rdparty/linenoise",
	}

	files {
		MAME_DIR .. "3rdparty/linenoise/utf8.c",
		MAME_DIR .. "3rdparty/linenoise/linenoise.c",
	}


--------------------------------------------------
-- utf8proc library objects
--------------------------------------------------

if not _OPTIONS["with-system-utf8proc"] then
project "utf8proc"
	uuid "1f881f09-0395-4483-ac37-2935fb092187"
	kind "StaticLib"

	defines {
		"UTF8PROC_STATIC",
	}

	configuration "Debug"
		defines {
			"verbose=-1",
		}

	configuration { "gmake or ninja" }
		buildoptions_c {
			"-Wno-strict-prototypes",
		}

	configuration { }

	files {
		MAME_DIR .. "3rdparty/utf8proc/utf8proc.c"
	}
else
links {
	ext_lib("utf8proc"),
}
end


--------------------------------------------------
-- wdlfft library objects (from Cockos WDL)
--------------------------------------------------

project "wdlfft"
	uuid "74ca017e-fa0d-48b8-81d6-8081a37be14c"
	kind "StaticLib"

	configuration { "gmake or ninja" }
		buildoptions_c {
			"-Wno-strict-prototypes",
		}

	configuration { }

	files {
		MAME_DIR .. "3rdparty/wdlfft/fft.c",
		MAME_DIR .. "3rdparty/wdlfft/fft.h"
	}


--------------------------------------------------
-- asmjit library
--------------------------------------------------

if not _OPTIONS["FORCE_DRC_C_BACKEND"] then
project "asmjit"
	uuid "4539757c-6e99-4bae-b3d0-b342a7c49539"
	kind "StaticLib"

	configuration { "gmake" }
		buildoptions {
			"-Wno-unused-but-set-variable",
		}

	configuration { }

	if _OPTIONS["targetos"]=="macosx" and _OPTIONS["gcc"]~=nil then
		if string.find(_OPTIONS["gcc"], "clang") and (version < 80000) then
			defines {
				"TARGET_OS_OSX=1",
			}
		end
	end

	files {
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/asmjit.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/api-build_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/api-config.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/arch.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/arch.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/assembler.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/assembler.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/builder.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/builder.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/callconv.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/callconv.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/codebuffer.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/codebufferwriter_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/codeholder.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/codeholder.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/compiler.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/compiler.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/constpool.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/constpool.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/cpuinfo.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/cpuinfo.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/datatypes.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/emitter.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/emitter.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/emitterutils.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/emitterutils_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/environment.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/environment.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/errorhandler.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/errorhandler.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/features.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/formatter.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/formatter.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/func.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/func.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/globals.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/globals.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/inst.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/inst.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/jitallocator.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/jitallocator.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/jitruntime.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/jitruntime.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/logger.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/logger.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/misc_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/operand.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/operand.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/osutils.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/osutils.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/osutils_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/raassignment_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/rabuilders_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/radefs_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/ralocal.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/ralocal_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/rapass.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/rapass_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/rastack.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/rastack_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/string.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/string.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/support.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/support.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/target.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/target.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/type.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/type.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/virtmem.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/virtmem.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zone.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zone.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonehash.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonehash.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonelist.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonelist.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonestack.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonestack.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonestring.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonetree.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonetree.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonevector.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/core/zonevector.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86archdata.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86archdata_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86assembler.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86assembler.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86builder.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86builder.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86callconv.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86callconv_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86compiler.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86compiler.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86emitter.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86features.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86features.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86formatter.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86formatter_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86globals.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86instapi.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86instapi_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86instdb.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86instdb.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86instdb_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86internal.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86internal_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86opcode_p.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86operand.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86operand.h",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86rapass.cpp",
		MAME_DIR .. "3rdparty/asmjit/src/asmjit/x86/x86rapass_p.h",
	}
end
