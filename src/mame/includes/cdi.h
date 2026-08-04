// license:BSD-3-Clause
// copyright-holders:Ryan Holtz

#ifndef MAME_PHILIPS_CDI_H
#define MAME_PHILIPS_CDI_H

#include "machine/scc68070.h"
#include "machine/cdislavehle.h"
#include "machine/cdicdic.h"
#include "sound/dmadac.h"
#include "video/mcd212.h"
#include "cpu/m6805/m68hc05.h"
#include "diserial.h"
#include "screen.h"

/*----------- driver state -----------*/

class cdi_state : public driver_device
{
public:
	cdi_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_main_rom(*this, "maincpu")
		, m_lcd(*this, "lcd")
		, m_slave_hle(*this, "slave_hle")
		, m_plane_ram(*this, "plane%u", 0U)
		, m_servo(*this, "servo")
		, m_slave(*this, "slave")
		, m_cdic(*this, "cdic")
		, m_cdrom(*this, "cdrom")
		, m_mcd212(*this, "mcd212")
		, m_dmadac(*this, "dac%u", 1U)
	{ }

	void cdimono1_base(machine_config &config);
	void cdimono1(machine_config &config);
	void cdimono2(machine_config &config);
	void cdi910(machine_config &config);

protected:
	enum servo_portc_bit_t
	{
		INV_JUC_OUT = (1 << 2),
		INV_DIV4_IN = (1 << 5),
		INV_CADDYSWITCH_IN = (1 << 7)
	};

	required_device<scc68070_device> m_maincpu;
	required_region_ptr<uint16_t> m_main_rom;
	optional_device<screen_device> m_lcd;
	optional_device<cdislave_hle_device> m_slave_hle;
	required_shared_ptr_array<uint16_t, 2> m_plane_ram;
	optional_device<m68hc05c8_device> m_servo;
	optional_device<m68hc05c8_device> m_slave;
	optional_device<cdicdic_device> m_cdic;
	required_device<cdrom_image_device> m_cdrom;
	required_device<mcd212_device> m_mcd212;

	required_device_array<dmadac_sound_device, 2> m_dmadac;

	uint32_t screen_update_cdimono1_lcd(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	virtual void machine_reset() override ATTR_COLD;

	void cdimono1_mem(address_map &map) ATTR_COLD;

	void cdi910_mem(address_map &map) ATTR_COLD;
	void cdimono2_mem(address_map &map) ATTR_COLD;
	void cdi070_cpuspace(address_map &map) ATTR_COLD;

	template<int Channel> uint16_t plane_r(offs_t offset, uint16_t mem_mask = ~0);
	template<int Channel> void plane_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint16_t main_rom_r(offs_t offset);

	uint16_t dvc_r(offs_t offset, uint16_t mem_mask = ~0);
	void dvc_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint16_t bus_error_r(offs_t offset);
	void bus_error_w(offs_t offset, uint16_t data);
};

#endif // MAME_PHILIPS_CDI_H
