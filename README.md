# **SAME_CDI** #
![SAME_CDI](https://i.imgur.com/hLyVKCp.png)

SAME_CDI is a ***S***ingle ***A***rcade/***M***achine ***E***mulator for libretro, forked from MAME libretro, which is a fork of MAME (it's the ***SAME***, get it?), and compiles only the Philips CD-I driver, and then sorts out the claptrap of loading the CD file for emulating.  The name removes the name MAME altogether, as thats what I gather the MAME folks want for a fork like this - MAMEs license is below.

*GAME FILES*
=======
Game files can be either CHD, ISO, or BIN/CUE.  Game support/functionality is whatever the MAME version included supports.

*BIOS*
=======
BIOS file (cdimono1.zip) is required, and can go either in the same directory as your ```CHD|ISO|CUE``` files or in the ```retroarch_system_dir/same_cdi/bios/``` directory

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
