// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
/******************************************************************************


    Philips CD-i consoles and games
    -------------------------------

    Preliminary MAME driver by Ryan Holtz
    Help provided by CD-i Fan


*******************************************************************************

STATUS:

  CD-i:
- The SLAVE MCU cannot be low-level emulated until there is proper /DTACK
  support in the 68k core. A2/A1 and D7..D0 are hooked up to Port C bits 1/0
  and Port A respectively, the Read/Write signal is sent to Port D bit 7, and
  /DTACK is received from Port B bit 6. The MCU therefore has the capability to
  pull /DTACK high on a data read in order to tell the 68k to hold off until
  data is ready.

- There is currently a lack of documentation on any of the chips used for
  audio in any of the CD-i models. The CDIC, which was used on Mono-I boards,
  is partially emulated thanks to information provided by CD-i Fan, the author
  of CD-i Emu. Desired documentation includes:
  * GSX38KG307CE46, "ATTEX"
  * Philips IMS66490, "CDIC" ADPCM decoder
  * PC85010 DSP

TODO:

- Screen clocks are a hack right now; they should be exactly CLOCK_A/2. However, the
  MCD-212 documentation states in both tables and timing diagrams that vertical retrace
  has an additional half-line even in non-interlaced mode, which cannot be represented
  in the current screen-timing framework. The input clock has been adjusted downward
  to factor out this half-line, resulting in the expected 50Hz exactly in PAL mode.

- Proper abstraction of the 68070's internal devices (UART, DMA, Timers, etc.)

- Mono-I: Full emulation of the CDIC, as well as the SERVO and SLAVE MCUs

- Mono-II: SERVO and SLAVE I/O device hookup
- Mono-II: DSP56k hookup

*******************************************************************************/

#include "emu.h"
#include "includes/cdi.h"

#include "cpu/m6805/m6805.h"
#include "imagedev/chd_cd.h"
#include "machine/timekpr.h"
#include "sound/cdda.h"

#include "emupal.h"
#include "screen.h"
#include "softlist.h"
#include "speaker.h"

#include "cdrom.h"

#include "cdi.lh"

// TODO: NTSC system clock is 30.2098 MHz; additional 4.9152 MHz XTAL provided for UART
#define CLOCK_A 30_MHz_XTAL

#define LOG_DVC             (1 << 1)
#define LOG_UART            (1 << 5)

#define VERBOSE         (0)
#include "logmacro.h"

#define ENABLE_UART_PRINTING (0)

/*************************
*      Memory maps       *
*************************/

void cdi_state::cdimono1_mem(address_map &map)
{
	map(0x000000, 0xffffff).rw(FUNC(cdi_state::bus_error_r), FUNC(cdi_state::bus_error_w));
	map(0x000000, 0x07ffff).rw(FUNC(cdi_state::plane_r<0>), FUNC(cdi_state::plane_w<0>)).share("plane0");
	map(0x200000, 0x27ffff).rw(FUNC(cdi_state::plane_r<1>), FUNC(cdi_state::plane_w<1>)).share("plane1");
	map(0x300000, 0x303bff).rw(m_cdic, FUNC(cdicdic_device::ram_r), FUNC(cdicdic_device::ram_w));
#if ENABLE_UART_PRINTING
	map(0x301400, 0x301403).r(m_maincpu, FUNC(scc68070_device::uart_loopback_enable));
#endif
	map(0x303c00, 0x303fff).rw(m_cdic, FUNC(cdicdic_device::regs_r), FUNC(cdicdic_device::regs_w));
	map(0x310000, 0x317fff).rw(m_slave_hle, FUNC(cdislave_hle_device::slave_r), FUNC(cdislave_hle_device::slave_w));
	map(0x318000, 0x31ffff).noprw();
	map(0x320000, 0x323fff).rw("mk48t08", FUNC(timekeeper_device::read), FUNC(timekeeper_device::write)).umask16(0xff00);    /* nvram (only low bytes used) */
	map(0x400000, 0x47ffff).r(FUNC(cdi_state::main_rom_r));
	map(0x4fffe0, 0x4fffff).m(m_mcd212, FUNC(mcd212_device::map));
	map(0x500000, 0x57ffff).ram();
	map(0xd00000, 0xdfffff).ram(); // DVC RAM block 1
	map(0xe00000, 0xe7ffff).rw(FUNC(cdi_state::dvc_r), FUNC(cdi_state::dvc_w));
	map(0xe80000, 0xefffff).ram(); // DVC RAM block 2
}

void cdi_state::cdimono2_mem(address_map &map)
{
	map(0x000000, 0x07ffff).ram().share("plane0");
	map(0x200000, 0x27ffff).ram().share("plane1");
#if ENABLE_UART_PRINTING
	map(0x301400, 0x301403).r(m_maincpu, FUNC(scc68070_device::uart_loopback_enable));
#endif
	map(0x320000, 0x323fff).rw("mk48t08", FUNC(timekeeper_device::read), FUNC(timekeeper_device::write)).umask16(0xff00);    /* nvram (only low bytes used) */
	map(0x400000, 0x47ffff).r(FUNC(cdi_state::main_rom_r));
	map(0x4fffe0, 0x4fffff).m(m_mcd212, FUNC(mcd212_device::map));
}

void cdi_state::cdi910_mem(address_map &map)
{
	map(0x000000, 0x07ffff).ram().share("plane0");
	map(0x180000, 0x1fffff).rom().region("maincpu", 0); // boot vectors point here
	map(0x200000, 0x27ffff).ram().share("plane1");
#if ENABLE_UART_PRINTING
	map(0x301400, 0x301403).r(m_maincpu, FUNC(scc68070_device::uart_loopback_enable));
#endif
	map(0x320000, 0x323fff).rw("mk48t08", FUNC(timekeeper_device::read), FUNC(timekeeper_device::write)).umask16(0xff00);    /* nvram (only low bytes used) */
	map(0x4fffe0, 0x4fffff).m(m_mcd212, FUNC(mcd212_device::map));
	map(0x500000, 0xffffff).noprw();
}


/*************************
*      Input ports       *
*************************/

static INPUT_PORTS_START( cdi )
INPUT_PORTS_END

static INPUT_PORTS_START( cdimono2 )
INPUT_PORTS_END

/***************************
*  Machine Initialization  *
***************************/

void cdi_state::machine_reset()
{
	uint16_t *src = &m_main_rom[0];
	uint16_t *dst = &m_plane_ram[0][0];
	memcpy(dst, src, 0x8);
}

/***************************
*  Wait-State Handling     *
***************************/

template<int Channel>
uint16_t cdi_state::plane_r(offs_t offset, uint16_t mem_mask)
{
	m_maincpu->eat_cycles(m_mcd212->ram_dtack_cycle_count<Channel>());
	return m_plane_ram[Channel][offset];
}

template<int Channel>
void cdi_state::plane_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	m_maincpu->eat_cycles(m_mcd212->ram_dtack_cycle_count<Channel>());
	COMBINE_DATA(&m_plane_ram[Channel][offset]);
}

uint16_t cdi_state::main_rom_r(offs_t offset)
{
	m_maincpu->eat_cycles(m_mcd212->rom_dtack_cycle_count());
	return m_main_rom[offset];
}


/**********************
*  BERR Handling      *
**********************/

uint16_t cdi_state::bus_error_r(offs_t offset)
{
	if(!machine().side_effects_disabled())
	{
		m_maincpu->set_buserror_details(offset*2, true, m_maincpu->get_fc());
		m_maincpu->set_input_line(M68K_LINE_BUSERROR, ASSERT_LINE);
		m_maincpu->set_input_line(M68K_LINE_BUSERROR, CLEAR_LINE);
	}
	return 0xff;
}

void cdi_state::bus_error_w(offs_t offset, uint16_t data)
{
	if(!machine().side_effects_disabled())
	{
		m_maincpu->set_buserror_details(offset*2, false, m_maincpu->get_fc());
		m_maincpu->set_input_line(M68K_LINE_BUSERROR, ASSERT_LINE);
		m_maincpu->set_input_line(M68K_LINE_BUSERROR, CLEAR_LINE);
	}
}

/*************************
*     DVC cartridge      *
*************************/

uint16_t cdi_state::dvc_r(offs_t offset, uint16_t mem_mask)
{
	return 0;
}

void cdi_state::dvc_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
}

/*************************
*       LCD screen       *
*************************/

static const uint16_t cdi220_lcd_char[20*22] =
{
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x8000, 0x8000, 0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000, 0x0002, 0x0002, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x8000, 0x8000, 0x8000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0002, 0x0002, 0x0002, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0002, 0x0002, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0000, 0x8000, 0x8000, 0x8000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0002, 0x0000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x0000, 0x0000, 0x8000, 0x8000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0002, 0x0002, 0x0000, 0x0000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x2000, 0x2000, 0x2000, 0x2000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0200, 0x0200, 0x0200, 0x0200,
	0x1000, 0x1000, 0x1000, 0x1000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x4000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0000, 0x0000, 0x0010, 0x0010, 0x0001, 0x0001, 0x0001, 0x0001, 0x0008, 0x0008, 0x0000, 0x0000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0000, 0x0010, 0x0010, 0x0010, 0x0001, 0x0001, 0x0001, 0x0001, 0x0008, 0x0008, 0x0008, 0x0000, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0010, 0x0010, 0x0010, 0x0010, 0x0001, 0x0001, 0x0001, 0x0001, 0x0008, 0x0008, 0x0008, 0x0008, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0010, 0x0010, 0x0010, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0008, 0x0008, 0x0008, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0010, 0x0010, 0x0000, 0x0000, 0x0001, 0x0001, 0x0001, 0x0001, 0x0000, 0x0000, 0x0008, 0x0008, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400,
	0x1000, 0x1000, 0x1000, 0x1000, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0800, 0x0400, 0x0400, 0x0400, 0x0400
};

uint32_t cdi_state::screen_update_cdimono1_lcd(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	if (!m_slave_hle.found())
		return 0;

	for (int y = 0; y < 22; y++)
	{
		uint32_t *scanline = &bitmap.pix(y);

		for (int lcd = 0; lcd < 8; lcd++)
		{
			uint16_t data = (m_slave_hle->get_lcd_state()[lcd*2] << 8) |
							m_slave_hle->get_lcd_state()[lcd*2 + 1];
			for (int x = 0; x < 20; x++)
			{
				if (data & cdi220_lcd_char[y*20 + x])
				{
					scanline[(7 - lcd)*24 + x] = rgb_t::white();
				}
				else
				{
					scanline[(7 - lcd)*24 + x] = rgb_t::black();
				}
			}
		}
	}

	return 0;
}

/*************************
*    Machine Drivers     *
*************************/

// CD-i Mono-I system base
void cdi_state::cdimono1_base(machine_config &config)
{
	SCC68070(config, m_maincpu, CLOCK_A);
	m_maincpu->set_addrmap(AS_PROGRAM, &cdi_state::cdimono1_mem);
	m_maincpu->iack4_callback().set(m_cdic, FUNC(cdicdic_device::intack_r));

	MCD212(config, m_mcd212, CLOCK_A, m_plane_ram[0], m_plane_ram[1]);
	m_mcd212->set_screen("screen");
	m_mcd212->int_callback().set(m_maincpu, FUNC(scc68070_device::int1_w));

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(14976000, 960, 0, 768, 312, 32, 312);
	screen.set_video_attributes(VIDEO_UPDATE_SCANLINE);
	screen.set_screen_update(m_mcd212, FUNC(mcd212_device::screen_update));

	SCREEN(config, m_lcd, SCREEN_TYPE_RASTER);
	m_lcd->set_refresh_hz(50);
	m_lcd->set_vblank_time(ATTOSECONDS_IN_USEC(0));
	m_lcd->set_size(192, 22);
	m_lcd->set_visarea(0, 192-1, 0, 22-1);
	m_lcd->set_screen_update(FUNC(cdi_state::screen_update_cdimono1_lcd));

	PALETTE(config, "palette").set_entries(0x100);

	config.set_default_layout(layout_cdi);

	// IMS66490 CDIC input clocks are 22.5792 MHz and 19.3536 MHz
	// DSP input clock is 7.5264 MHz
	CDI_CDIC(config, m_cdic, 45.1584_MHz_XTAL / 2);
	m_cdic->set_clock2(45.1584_MHz_XTAL * 3 / 7); // generated by PLL circuit incorporating 19.3575 MHz XTAL
	m_cdic->intreq_callback().set(m_maincpu, FUNC(scc68070_device::in4_w));

	CDI_SLAVE_HLE(config, m_slave_hle, 0);
	m_slave_hle->int_callback().set(m_maincpu, FUNC(scc68070_device::in2_w));

	/* sound hardware */
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	DMADAC(config, m_dmadac[0]);
	m_dmadac[0]->add_route(ALL_OUTPUTS, "lspeaker", 1.0);

	DMADAC(config, m_dmadac[1]);
	m_dmadac[1]->add_route(ALL_OUTPUTS, "rspeaker", 1.0);

	MK48T08(config, "mk48t08");
}

// CD-i model 220 (Mono-II, NTSC)
void cdi_state::cdimono2(machine_config &config)
{
	SCC68070(config, m_maincpu, CLOCK_A);
	m_maincpu->set_addrmap(AS_PROGRAM, &cdi_state::cdimono2_mem);

	MCD212(config, m_mcd212, CLOCK_A, m_plane_ram[0], m_plane_ram[1]);
	m_mcd212->set_screen("screen");
	m_mcd212->int_callback().set(m_maincpu, FUNC(scc68070_device::int1_w));

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(14976000, 960, 0, 768, 312, 32, 312);
	screen.set_video_attributes(VIDEO_UPDATE_SCANLINE);
	screen.set_screen_update(m_mcd212, FUNC(mcd212_device::screen_update));

	SCREEN(config, m_lcd, SCREEN_TYPE_RASTER);
	m_lcd->set_refresh_hz(60);
	m_lcd->set_vblank_time(ATTOSECONDS_IN_USEC(0));
	m_lcd->set_size(192, 22);
	m_lcd->set_visarea(0, 192-1, 0, 22-1);
	m_lcd->set_screen_update(FUNC(cdi_state::screen_update_cdimono1_lcd));

	PALETTE(config, "palette").set_entries(0x100);

	config.set_default_layout(layout_cdi);

	M68HC05C8(config, m_servo, 4_MHz_XTAL);
	M68HC05C8(config, m_slave, 4_MHz_XTAL);

	CDROM(config, "cdrom").set_interface("cdi_cdrom");
	SOFTWARE_LIST(config, "cd_list").set_original("cdi").set_filter("!DVC");

	/* sound hardware */
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	DMADAC(config, m_dmadac[0]);
	m_dmadac[0]->add_route(ALL_OUTPUTS, "lspeaker", 1.0);

	DMADAC(config, m_dmadac[1]);
	m_dmadac[1]->add_route(ALL_OUTPUTS, "rspeaker", 1.0);

	MK48T08(config, "mk48t08");
}

void cdi_state::cdi910(machine_config &config)
{
	SCC68070(config, m_maincpu, CLOCK_A);
	m_maincpu->set_addrmap(AS_PROGRAM, &cdi_state::cdi910_mem);

	MCD212(config, m_mcd212, CLOCK_A, m_plane_ram[0], m_plane_ram[1]);
	m_mcd212->set_screen("screen");
	m_mcd212->int_callback().set(m_maincpu, FUNC(scc68070_device::int1_w));

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(14976000, 960, 0, 768, 312, 32, 312);
	screen.set_video_attributes(VIDEO_UPDATE_SCANLINE);
	screen.set_screen_update(m_mcd212, FUNC(mcd212_device::screen_update));

	SCREEN(config, m_lcd, SCREEN_TYPE_RASTER);
	m_lcd->set_refresh_hz(60);
	m_lcd->set_vblank_time(ATTOSECONDS_IN_USEC(0));
	m_lcd->set_size(192, 22);
	m_lcd->set_visarea(0, 192-1, 0, 22-1);
	m_lcd->set_screen_update(FUNC(cdi_state::screen_update_cdimono1_lcd));

	PALETTE(config, "palette").set_entries(0x100);

	config.set_default_layout(layout_cdi);

	M68HC05C8(config, m_servo, 4_MHz_XTAL);
	M68HC05C8(config, m_slave, 4_MHz_XTAL);

	CDROM(config, "cdrom").set_interface("cdi_cdrom");
	SOFTWARE_LIST(config, "cd_list").set_original("cdi").set_filter("!DVC");

	/* sound hardware */
	SPEAKER(config, "lspeaker").front_left();
	SPEAKER(config, "rspeaker").front_right();

	DMADAC(config, m_dmadac[0]);
	m_dmadac[0]->add_route(ALL_OUTPUTS, "lspeaker", 1.0);

	DMADAC(config, m_dmadac[1]);
	m_dmadac[1]->add_route(ALL_OUTPUTS, "rspeaker", 1.0);

	MK48T08(config, "mk48t08");
}

// CD-i Mono-I, with CD-ROM image device (MESS) and Software List (MESS)
void cdi_state::cdimono1(machine_config &config)
{
	cdimono1_base(config);

	CDROM(config, "cdrom").set_interface("cdi_cdrom");
	SOFTWARE_LIST(config, "cd_list").set_original("cdi").set_filter("!DVC");
}

/*************************
*        Rom Load        *
*************************/

ROM_START( cdimono1 )
	ROM_REGION(0x80000, "maincpu", 0) // these roms need byteswapping
	ROM_SYSTEM_BIOS( 0, "mcdi200", "Magnavox CD-i 200" )
	ROMX_LOAD( "cdi200.rom", 0x000000, 0x80000, CRC(40c4e6b9) SHA1(d961de803c89b3d1902d656ceb9ce7c02dccb40a), ROM_BIOS(0) )
	ROM_SYSTEM_BIOS( 1, "pcdi220", "Philips CD-i 220 F2" )
	ROMX_LOAD( "cdi220b.rom", 0x000000, 0x80000, CRC(279683ca) SHA1(53360a1f21ddac952e95306ced64186a3fc0b93e), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 2, "pcdi220_alt", "Philips CD-i 220?" ) // doesn't boot
	ROMX_LOAD( "cdi220.rom", 0x000000, 0x80000, CRC(584c0af8) SHA1(5d757ab46b8c8fc36361555d978d7af768342d47), ROM_BIOS(2) )

	// The two MCU dumps below are taken from the cdi910. We still need dumps from a Mono-I board in case the revisions are different.
	ROM_REGION(0x2000, "servo", 0)
	ROM_LOAD( "zx405037p__cdi_servo_2.1__b43t__llek9215.mc68hc705c8a_withtestrom.7201", 0x0000, 0x2000, CRC(7a3af407) SHA1(fdf8d78d6a0df4a56b5b963d72eabd39fcec163f) BAD_DUMP )

	ROM_REGION(0x2000, "slave", 0)
	ROM_LOAD( "zx405042p__cdi_slave_2.0__b43t__zzmk9213.mc68hc705c8a_withtestrom.7206", 0x0000, 0x2000, CRC(688cda63) SHA1(56d0acd7caad51c7de703247cd6d842b36173079) BAD_DUMP )
ROM_END

ROM_START( cdi910 )
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "cdi910", "CD-I 910-17P Mini-MMC" )
	ROMX_LOAD( "philips__cd-i_2.1__mb834200b-15__26b_aa__9224_z01.tc574200.7211", 0x000000, 0x80000, CRC(4ae3bee3) SHA1(9729b4ee3ce0c17172d062339c47b1ab822b222b), ROM_BIOS(0) | ROM_GROUPWORD | ROM_REVERSE )
	ROM_SYSTEM_BIOS( 1, "cdi910_alt", "alt" )
	ROMX_LOAD( "cdi910.rom", 0x000000, 0x80000, CRC(2f3048d2) SHA1(11c4c3e602060518b52e77156345fa01f619e793), ROM_BIOS(1) | ROM_GROUPWORD | ROM_REVERSE )

	ROM_REGION(0x2000, "servo", 0)
	ROM_LOAD( "zx405037p__cdi_servo_2.1__b43t__llek9215.mc68hc705c8a_withtestrom.7201", 0x0000, 0x2000, CRC(7a3af407) SHA1(fdf8d78d6a0df4a56b5b963d72eabd39fcec163f) )

	ROM_REGION(0x2000, "slave", 0)
	ROM_LOAD( "zx405042p__cdi_slave_2.0__b43t__zzmk9213.mc68hc705c8a_withtestrom.7206", 0x0000, 0x2000, CRC(688cda63) SHA1(56d0acd7caad51c7de703247cd6d842b36173079) )

	ROM_REGION(0x2000, "pals", 0)
	ROM_LOAD( "ti_portugal_206xf__tibpal20l8-15cnt__m7205n.7205.bin",      0x0000, 0x144, CRC(dd167e0d) SHA1(2ba82a4619d7a0f19e62e02a2841afd4d45d56ba) )
	ROM_LOAD( "ti_portugal_774_206xf__tibpal16l8-10cn_m7204n.7204.bin",    0x0000, 0x104, CRC(04e6bd37) SHA1(153d1a977291bedb7420484a9f889325dbd3628e) )
ROM_END

ROM_START( cdimono2 )
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_LOAD16_WORD_SWAP( "philips__cdi-220_ph3_r1.2__mb834200b-15__02f_aa__9402_z04.tc574200-le._1.7211", 0x000000, 0x80000, CRC(17d723e7) SHA1(6c317a82e35d60ca5e7a74fc99f665055693169d) )

	ROM_REGION(0x2000, "servo", 0)
	ROM_LOAD( "zc405351p__servo_cdi_4.1__0d67p__lluk9404.mc68hc705c8a.7490", 0x0000, 0x2000, CRC(2bc8e4e9) SHA1(8cd052b532fc052d6b0077261c12f800e8655bb1) )

	ROM_REGION(0x2000, "slave", 0)
	ROM_LOAD( "zc405352p__slave_cdi_4.1__0d67p__lltr9403.mc68hc705c8a.7206", 0x0000, 0x2000, CRC(5b19da07) SHA1(cf02d84977050c71e87a38f1249e83c43a93949b) )
ROM_END

ROM_START( cdi490a )
	ROM_REGION(0x80000, "maincpu", 0)
	ROM_SYSTEM_BIOS( 0, "cdi490", "CD-i 490" )
	ROMX_LOAD( "cdi490a.rom", 0x000000, 0x80000, CRC(e2f200f6) SHA1(c9bf3c4c7e4fe5cbec3fe3fc993c77a4522ca547), ROM_BIOS(0) | ROM_GROUPWORD | ROM_REVERSE  )

	ROM_REGION(0x40000, "mpegs", 0) // keep these somewhere
	ROM_LOAD( "impega.rom", 0x0000, 0x40000, CRC(84d6f6aa) SHA1(02526482a0851ea2a7b582d8afaa8ef14a8bd914) )
	ROM_LOAD( "vmpega.rom", 0x0000, 0x40000, CRC(db264e8b) SHA1(be407fbc102f1731a0862554855e963e5a47c17b) )
ROM_END


/*************************
*      Game driver(s)    *
*************************/

/*    YEAR  NAME      PARENT  COMPAT  MACHINE   INPUT     CLASS      INIT        COMPANY       FULLNAME */
// BIOS / System
CONS( 1991, cdimono1, 0,      0,      cdimono1, cdi,      cdi_state, empty_init, "Philips",    "CD-i (Mono-I) (PAL)",   MACHINE_IMPERFECT_GRAPHICS | MACHINE_IMPERFECT_SOUND | MACHINE_SUPPORTS_SAVE )
CONS( 1991, cdimono2, 0,      0,      cdimono2, cdimono2, cdi_state, empty_init, "Philips",    "CD-i (Mono-II) (NTSC)",   MACHINE_NOT_WORKING )
CONS( 1991, cdi910,   0,      0,      cdi910,   cdimono2, cdi_state, empty_init, "Philips",    "CD-i 910-17P Mini-MMC (PAL)",   MACHINE_NOT_WORKING )
CONS( 1991, cdi490a,  0,      0,      cdimono1, cdi,      cdi_state, empty_init, "Philips",    "CD-i 490",   MACHINE_NOT_WORKING )
