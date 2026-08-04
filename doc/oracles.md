# Accuracy oracles

Reference implementations and measurements to verify same_cdi behavior
against, in order of preference.

## CDi_MiSTer (FPGA implementation)

https://github.com/MiSTer-devel/CDi_MiSTer

Mono-I (210/05-class) FPGA implementation, actively developed, validated
per-title against a real CD-i 210/05 and against the BIOS self test.  Its
`doc/` directory contains logic-analyzer measurements taken on real
hardware (MCD212 VideoIRQ position relative to VSync/VBlank, ICA/DCA
execution timing, Display Active edges) and a curated list of quirks
confirmed present on real hardware (`doc/quirks.md`) -- useful to avoid
"fixing" behavior that is authentic.

Where it is more accurate than this core (verified against our own
status headers):

| Subsystem          | same_cdi                                      | CDi_MiSTer                                                    |
|--------------------|-----------------------------------------------|---------------------------------------------------------------|
| SLAVE MCU          | HLE (`cdislavehle`, "TODO: Proper LLE")       | LLE: dumped slave 2.0 firmware on a 68HC05 core               |
| CDIC               | "just enough" HLE, DMA is high-level          | register-level model (cdifan RE), real DMA, hw-verified timing |
| MCD212             | scanline renderer, QHY DYUV unsupported       | fetch/decode pipeline, ICA/DCA timing measured on hardware    |
| 68070 peripherals  | "Skeleton" (UART/DMA/timers), magicard hacks  | implemented, DMA validated against OS-9 driver traces         |
| DVC (MPEG cart)    | absent                                        | implemented (VMPEG)                                           |

Where it is *not* automatically better: 68000 instruction semantics and
instruction timing (it uses TG68K.C with approximated 68070 timing; our
interpreter core is functionally better tested but its 68070 cycle
counts are approximate too), and the servo (HLE on both sides).

License note: the RTL is GPLv3.  Use it as a behavioral reference and
for its hardware measurements; do not port RTL verbatim into
BSD-3-Clause source files.

## cdifan / cdichips

https://github.com/cdifan/cdichips

Register-level reverse engineering of the CD-i ASICs (notably the
IMS66490 CDIC and MCD212) underlying both CD-i Emulator and the MiSTer
core.  Primary written source for register maps and buffer layouts.

## Real-hardware ground truth

CDi_MiSTer's `doc/quirks.md` documents title-level behavior reproduced
on a real 210/05 (micro-jitter in Zelda map scrolling, Myst transition
line glitches from mid-display VideoIRQ, per-title audio artifacts).
Treat entries there as authentic machine behavior, not emulation bugs.
