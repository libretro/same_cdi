// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
/******************************************************************************


    CD-i MCD212 video emulation
    -------------------

    written by Ryan Holtz


*******************************************************************************

STATUS:

- Just enough for the Mono-I CD-i board to work somewhat properly.

TODO:

- Unknown yet.

*******************************************************************************/

#ifndef MAME_PHILIPS_MCD212_H
#define MAME_PHILIPS_MCD212_H

#pragma once


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class mcd212_device : public device_t,
					  public device_video_interface
{
public:
	template <typename T, typename U>
	mcd212_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock, T &&plane_a_tag, U &&plane_b_tag)
		: mcd212_device(mconfig, tag, owner, clock)
	{
		m_planea.set_tag(std::forward<T>(plane_a_tag));
		m_planeb.set_tag(std::forward<U>(plane_b_tag));
	}

	mcd212_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto int_callback() { return m_int_callback.bind(); }

	// The CD-i's black.  Everything this chip emits carries the 16-235 studio
	// swing: the 4bpp constants below run 0x10 to 0xe6, and mix_lines() takes
	// the pedestal off each plane before weighting it and puts it back
	// afterwards.  Anything feeding pixels in from outside - the External Video
	// backdrop - has to speak the same units, or its black reads as a darker
	// hole in the picture.
	static constexpr uint32_t CDI_BLACK = 0x00101010;

	// Supplies the External Video backdrop, i.e. the picture coming from a
	// Digital Video Cartridge. Called with the active-area pixel position;
	// returns 0 when the cartridge's display window does not cover that pixel,
	// or the colour with the alpha byte set when it does. A covered pixel with
	// no picture behind it comes back CDI_BLACK rather than unoccupied, so that
	// the window still keys the CD-i plane away between clips.
	typedef device_delegate<uint32_t (int, int)> ext_video_delegate;
	template <typename... T> void set_ext_video_callback(T &&... args) { m_ext_video_cb.set(std::forward<T>(args)...); }

	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	void map(address_map &map) ATTR_COLD;

	template <int Path> int ram_dtack_cycle_count();
	int rom_dtack_cycle_count();

protected:
	// device_t implementation
	// device-level overrides
	virtual void device_resolve_objects() override;
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_post_load() override;

	TIMER_CALLBACK_MEMBER(ica_tick);
	TIMER_CALLBACK_MEMBER(dca_tick);

	uint8_t csr1_r();
	void csr1_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t dcr1_r(offs_t offset, uint16_t mem_mask = ~0);
	void dcr1_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t vsr1_r(offs_t offset, uint16_t mem_mask = ~0);
	void vsr1_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t ddr1_r(offs_t offset, uint16_t mem_mask = ~0);
	void ddr1_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t dca1_r(offs_t offset, uint16_t mem_mask = ~0);
	void dca1_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	uint8_t csr2_r();
	void csr2_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t dcr2_r(offs_t offset, uint16_t mem_mask = ~0);
	void dcr2_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t vsr2_r(offs_t offset, uint16_t mem_mask = ~0);
	void vsr2_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t ddr2_r(offs_t offset, uint16_t mem_mask = ~0);
	void ddr2_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t dca2_r(offs_t offset, uint16_t mem_mask = ~0);
	void dca2_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	enum : uint32_t
	{
		CURCNT_COLOR         = 0x00000f,    // Cursor color
		CURCNT_CUW           = 0x008000,    // Cursor width
		CURCNT_COF_SHIFT     = 16,
		CURCNT_COF           = 0b111 << CURCNT_COF_SHIFT,    // Cursor off time
		CURCNT_CON_SHIFT     = 19,
		CURCNT_CON           = 0b111 << CURCNT_CON_SHIFT,    // Cursor on time
		CURCNT_BLKC_SHIFT    = 22,
		CURCNT_BLKC          = 0x400000,    // Blink type
		CURCNT_EN            = 0x800000,    // Cursor enable

		ICM_MODE1            = 0x00000f,    // Plane 1
		ICM_MODE1_SHIFT      = 0,
		ICM_MODE2            = 0x000f00,    // Plane 2
		ICM_MODE2_SHIFT      = 8,
		ICM_EV               = 0x040000,    // External video
		ICM_EV_BIT           = 18,
		ICM_NM               = 0x080000,    // Number of Matte flags
		ICM_NM_BIT           = 19,
		ICM_CS               = 0x400000,    // CLUT select
		ICM_CS_BIT           = 22,

		TCR_TA               = 0x00000f,    // Plane A
		TCR_TB               = 0x000f00,    // Plane B
		TCR_TB_SHIFT         = 8,
		TCR_ALWAYS           = 0x0,         // Transparent if: Always (Plane Disabled)
		TCR_KEY              = 0x1,         // Transparent if: Color Key = True
		TCR_RGB              = 0x2,         // Transparent if: Transparency Bit = 1 (RGB Only)
		TCR_MF0              = 0x3,         // Transparent if: Matte Flag 0 = True
		TCR_MF1              = 0x4,         // Transparent if: Matte Flag 1 = True
		TCR_MF0_KEY1         = 0x5,         // Transparent if: Matte Flag 0 = True || Color Key = True
		TCR_MF1_KEY1         = 0x6,         // Transparent if: Matte Flag 1 = True || Color Key = True
		TCR_COND_UNUSED0     = 0x7,         // Unused
		TCR_NEVER            = 0x8,         // Transparent if: Never (No Transparent Area)
		TCR_NOT_KEY          = 0x9,         // Transparent if: Color Key = False
		TCR_NOT_RGB          = 0xa,         // Transparent if: Transparency Bit = 0 (RGB Only)
		TCR_NOT_MF0          = 0xb,         // Transparent if: Matte Flag 0 = False
		TCR_NOT_MF1          = 0xc,         // Transparent if: Matte Flag 1 = False
		TCR_NOT_MF0_KEY      = 0xd,         // Transparent if: Matte Flag 0 = False || Color Key = False
		TCR_NOT_MF1_KEY      = 0xe,         // Transparent if: Matte Flag 1 = False || Color Key = False
		TCR_COND_UNUSED1     = 0xf,         // Unused
		TCR_DISABLE_MX       = 0x800000,    // Mix disable

		POR_AB               = 0,           // Plane A in front of Plane B
		POR_BA               = 1,           // Plane B in front of Plane A

		MC_X                 = 0x0003ff,    // X position
		MC_WF                = 0x00fc00,    // Weight position
		MC_WF_SHIFT          = 10,
		MC_MF_BIT            = 16,          // Matte flag bit
		MC_OP                = 0xf00000,    // Operation
		MC_OP_SHIFT          = 20,

		CSR1R_PA             = 0x20,        // Parity
		CSR1R_DA             = 0x80,        // Display Active

		CSR1W_BE             = 0x0001,      // Bus Error
		CSR1W_ST_BIT         = 1,           // Standard
		CSR1W_DD_BIT         = 3,
		CSR1W_DD2            = 0x0300,      // /DTACK Delay
		CSR1W_DD2_SHIFT      = 8,
		CSR1W_DI1_BIT        = 15,

		CSR2R_BE             = 0x0001,      // Bus Error
		CSR2R_IT2            = 0x0002,      // Interrupt 2
		CSR2R_IT1            = 0x0004,      // Interrupt 1

		DCR_DCA_BIT          = 8,           // DCA Enable Ch.1/2
		DCR_ICA_BIT          = 9,           // ICA Enable Ch.1/2
		DCR_CM_BIT           = 11,          // Color Mode Ch.1/2
		DCR_SM_BIT           = 12,          // Scan Mode
		DCR_FD_BIT           = 13,          // Frame Duration
		DCR_CF_BIT           = 14,          // Crystal Frequency
		DCR_DE_BIT           = 15,          // Display Enable

		DDR_MT               = 0x0c00,      // Mosaic File Type
		DDR_MT_2             = 0x0000,      // 2x1
		DDR_MT_4             = 0x0400,      // 4x1
		DDR_MT_8             = 0x0800,      // 8x1
		DDR_MT_16            = 0x0c00,      // 16x1
		DDR_MT_SHIFT         = 10,
		DDR_FT               = 0x0300,      // Display File Type
		DDR_FT_BMP           = 0x0000,      // Bitmap
		DDR_FT_BMP2          = 0x0100,      // Bitmap (alt.)
		DDR_FT_RLE           = 0x0200,      // Run-Length Encoded
		DDR_FT_MOSAIC        = 0x0300,      // Mosaic

		ICM_OFF              = 0x0,
		ICM_CLUT8            = 0x1,
		ICM_RGB555           = 0x1,
		ICM_CLUT7            = 0x3,
		ICM_CLUT77           = 0x4,
		ICM_DYUV             = 0x5,
		ICM_CLUT4            = 0xb
	};

	uint8_t m_csrr[2]{};
	uint16_t m_csrw[2]{};
	uint16_t m_dcr[2]{};
	uint16_t m_vsr[2]{};
	uint16_t m_ddr[2]{};
	uint16_t m_dcp[2]{};
	uint32_t m_dca[2]{};
	uint32_t m_clut[256]{};
	uint32_t m_image_coding_method = 0;
	uint32_t m_transparency_control = 0;
	uint32_t m_plane_order = 0;
	uint32_t m_clut_bank[2]{};
	uint32_t m_transparent_color[2]{};
	uint32_t m_mask_color[2]{};
	uint32_t m_dyuv_abs_start[2]{};
	uint32_t m_cursor_position = 0;
	uint32_t m_cursor_control = 0;
	uint32_t m_cursor_pattern[16]{};
	uint32_t m_matte_control[8]{};
	uint32_t m_backdrop_color = 0;
	uint32_t m_mosaic_hold[2]{};
	uint8_t m_weight_factor[2][768]{};

	// DYUV color limit arrays.
	uint32_t m_dyuv_limit_lut[0x300];

	// DYUV delta-Y decoding array
	uint8_t m_delta_y_lut[0x100];

	// DYUV delta-UV decoding array
	uint8_t m_delta_uv_lut[0x100];

	// DYUV U-to-B decoding array
	int16_t m_dyuv_u_to_b[0x100];

	// U-to-G decoding array
	int16_t m_dyuv_u_to_g[0x100];

	// V-to-G decoding array
	int16_t m_dyuv_v_to_g[0x100];

	// V-to-R decoding array
	int16_t m_dyuv_v_to_r[0x100];

	// interrupt callbacks
	devcb_write_line m_int_callback;

	// External Video source (Digital Video Cartridge)
	ext_video_delegate m_ext_video_cb;

	required_shared_ptr<uint16_t> m_planea;
	required_shared_ptr<uint16_t> m_planeb;

	// internal state
	bool m_matte_flag[2][768]{};

	// The two weight-factor registers themselves, as distinct from the per-pixel
	// arrays the matte pass expands them into. Left out of the save state so
	// that existing states keep loading; device_post_load() reseeds them, and
	// the next ICA writes them outright.
	uint8_t m_weight_factor_reg[2]{};
	int m_ica_height = 0;
	int m_total_height = 0;
	emu_timer *m_ica_timer = nullptr;
	emu_timer *m_dca_timer = nullptr;

	// Cursor State
	uint16_t m_blink_time; // Counter that tracks how long since the last m_blink_active last changed.
	bool m_blink_active = false;

	static const uint32_t s_4bpp_color[16];

	uint8_t get_weight_factor(const uint32_t Matte_idx);
	uint8_t get_matte_op(const uint32_t Matte_idx);
	void update_matte_arrays();

	int get_screen_width();
	int get_border_width();
	uint32_t get_backdrop_plane(int x);
	void update_ext_video_line();

	int m_backdrop_scanline = 0;

	// The address the display is currently fetching each plane from.
	//
	// This is the chip's internal pointer, not the VSR register: VSR (with its
	// top six bits living in DCR) holds where a field *starts*, and the pointer
	// is loaded from it once per field and then walks forward as the line is
	// scanned out. Keeping the running position in the registers themselves
	// meant a CPU write to DCR part way down a field moved the fetch address
	// out from under the plane being drawn, because DCR's low six bits are the
	// high six bits of VSR. Monty Python does exactly that when it swaps
	// scenery in, and the field caught mid-write came out as bands of garbage.
	//
	// Deliberately not in the save state: it is reloaded from VSR at the top of
	// every field, so a state written at a frame boundary does not need it, and
	// leaving it out keeps existing save states loadable.
	uint32_t m_vsr_pos[2] = { 0, 0 };

	// The external-video picture for the scanline being drawn, sampled once
	// before the planes are decoded.  m_ext_present marks where the cartridge's
	// display window is open, not where it happens to have a picture.  Scratch
	// for one line, so deliberately not part of the save state.
	uint32_t m_ext_pixel[768]{};
	bool     m_ext_present[768]{};
	bool     m_ext_active = false;

	template <int Path> void set_vsr(uint32_t value);
	template <int Path> uint32_t get_vsr();

	template <int Path> void set_dcp(uint32_t value);
	template <int Path> uint32_t get_dcp();

	template <int Path> void set_display_parameters(uint8_t value);

	template <int Path> void process_ica();
	template <int Path> void process_dca();

	template <int Path> uint8_t get_transparency_control();
	template <int Path> uint8_t get_icm();
	template <int Path> bool get_mosaic_enable();
	template <int Path> uint8_t get_mosaic_factor();
	template <int Path> void process_vsr(uint32_t *pixels, bool *transparent);

	template <int Path> void set_register(uint8_t reg, uint32_t value);

	template <bool MosaicA, bool MosaicB, bool OrderAB> void mix_lines(uint32_t *plane_a, bool *transparent_a, uint32_t *plane_b, bool *transparent_b, uint32_t *out);

	void draw_cursor(uint32_t *scanline);
};

// device type definition
DECLARE_DEVICE_TYPE(MCD212, mcd212_device)

#endif // MAME_PHILIPS_MCD212_H
