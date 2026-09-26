# **SAME_CDI** #
![SAME_CDI](https://i.imgur.com/hLyVKCp.png)

SAME_CDI is a ***S***ingle ***A***rcade/***M***achine ***E***mulator for libretro, forked from MAME libretro, which is a fork of MAME (it's the ***SAME***, get it?), and compiles only the Philips CD-I driver, and then sorts out the claptrap of loading the CD file for emulating.  The name removes the name MAME altogether, as thats what I gather the MAME folks want for a fork like this - MAMEs license is below.

*GAME FILES*
=======
Game files can be either CHD, ISO, or BIN/CUE.  Game support/functionality is whatever the MAME version included supports.

*BIOS*
=======
BIOS file (cdimono1.zip) is required, and can go either in the same directory as your ```CHD|ISO|CUE``` files or in the ```retroarch_system_dir/same_cdi/bios/``` directory

*PAL AND NTSC*
=======
Select **Quick Menu > Core Options > System > Video standard (Restart)** to
choose PAL (the default) or NTSC, then close and reopen the content. Save a
game options override to keep NTSC for a particular title. The same
`cdimono1.zip` is used for both standards; `cdimono2.zip` describes a different,
unfinished motherboard and is not required for NTSC Mono-I playback.

The setting changes the player's SLAVE standard input, CPU/video clock,
display timing and Digital Video Cartridge standard. NTSC uses a 240-line
active field at approximately 59.94 Hz. The original Magnavox CD-i 200 BIOS
boots the unmodified NTSC release of *The Firm* with this setting. PAL and
NTSC states should be restored with the same video standard selected when
they were saved.

CD-i has no DVD-style region codes, but some CD-i Digital Video titles require
one television standard. This setting emulates that player configuration;
it does not alter disc images.

*DIGITAL VIDEO CARTRIDGE*
=======
Titles that need the Digital Video Cartridge for MPEG-1 full motion video,
such as *The Firm*, *Mutant Rampage: Bodyslam* and *Monty Python's Invasion
from the Planet Skyron*, also need the cartridge's driver ROM. Add
`vmpega.rom` (262144 bytes, CRC32 `db264e8b`) to `cdimono1.zip`; the same
file is part of `cdi490a.zip`, since the CD-i 490 has the hardware built in.
Without it the core runs as a plain Mono-I with no cartridge fitted, exactly
as before.

The cartridge is emulated from the reverse engineering done by the
[CDi_MiSTer](https://github.com/MiSTer-devel/CDi_MiSTer) project.

--------

# **Libretro notice** #

Before sending bug reports to the upstream bug tracker, make sure the bugs are reproducible in the latest standalone release.

To build libretro SAME_CDI core from source you need to use `Makefile.libretro` make file:

```
make -f Makefile.libretro
```

--------

License
=======
The MAME project as a whole is made available under the terms of the
[GNU General Public License, version 2](http://opensource.org/licenses/GPL-2.0)
or later (GPL-2.0+), since it contains code made available under multiple
GPL-compatible licenses.  A great majority of the source files (over 90%
including core files) are made available under the terms of the
[3-clause BSD License](http://opensource.org/licenses/BSD-3-Clause), and we
would encourage new contributors to make their contributions available under the
terms of this license.

The Digital Video Cartridge emulation (`src/mame/machine/cdidvc.*` and
`src/mame/machine/mpeg1demux.*`) is a port of the
[CDi_MiSTer](https://github.com/MiSTer-devel/CDi_MiSTer) VMPEG core and is
therefore made available under the terms of the
[GNU General Public License, version 3](https://opensource.org/licenses/GPL-3.0)
(GPL-3.0), like the project it was ported from. Every other license in the
tree is compatible with it, and binaries of this core that include those files
are distributed under GPL-3.0.

Please note that MAME is a registered trademark of Gregory Ember, and permission
is required to use the "MAME" name, logo, or wordmark.

<a href="http://opensource.org/licenses/GPL-2.0" target="_blank">
<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">
</a>

    Copyright (C) 1997-2021  MAMEDev and contributors

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License version 2, as provided in
    docs/legal/GPL-2.0.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details.

Please see COPYING for more details.
