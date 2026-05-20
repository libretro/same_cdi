// license:BSD-3-Clause
// copyright-holders:Ryan Holtz

#ifndef MAME_MACHINE_CDIDVC_H
#define MAME_MACHINE_CDIDVC_H

#pragma once

#include "machine/scc68070.h"

#include <memory>


// ======================> cdidvc_device

class cdidvc_device : public device_t
{
public:
	cdidvc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto intreq_callback() { return m_intreq_callback.bind(); }

	uint16_t regs_r(offs_t offset, uint16_t mem_mask = ~0);
	void regs_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint16_t mpeg_ram_r(offs_t offset, uint16_t mem_mask = ~0);
	void mpeg_ram_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	uint8_t intack_r();

protected:
	virtual void device_resolve_objects() override;
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

private:
	static constexpr device_timer_id TIMER_DCLK = 0;

	enum : uint16_t
	{
		FMV_INT_SEQ   = 0x0001,
		FMV_INT_GOP   = 0x0002,
		FMV_INT_PIC   = 0x0004,
		FMV_INT_EOD   = 0x0008,
		FMV_INT_RFB   = 0x0010,
		FMV_INT_NDAT  = 0x0020,
		FMV_INT_OVF   = 0x0040,
		FMV_INT_DCL   = 0x0080,
		FMV_INT_TIM   = 0x0100,
		FMV_INT_ESI   = 0x0200,
		FMV_INT_EII   = 0x0400,
		FMV_INT_VSYNC = 0x0800,
		FMV_INT_PAI   = 0x1000,
		FMV_INT_VCUP  = 0x2000,
		FMV_INT_ERDD  = 0x4000,
		FMV_INT_ERDV  = 0x8000
	};

	enum : uint16_t
	{
		FMA_INT_EOI  = 0x0001,
		FMA_INT_CSU  = 0x0002,
		FMA_INT_UPD  = 0x0004,
		FMA_INT_UNF  = 0x0008,
		FMA_INT_DEC  = 0x0010,
		FMA_INT_ERR  = 0x0020,
		FMA_INT_POLL = 0x0100
	};

	uint16_t read_rom(offs_t byte_offset) const;
	void update_interrupt_state();
	void dclk_tick();
	void complete_dma(int channel);
	void process_fma_dma();
	void process_fmv_dma();
	uint32_t make_timecode() const;
	bool mpeg_ram_enabled() const;
	void trigger_bus_error(offs_t offset, bool read);

	devcb_write_line m_intreq_callback;
	required_address_space m_memory_space;
	required_device<scc68070_device> m_scc;
	optional_region_ptr<uint8_t> m_rom;

	emu_timer *m_dclk_timer;
	std::unique_ptr<uint16_t[]> m_mpeg_ram;

	bool m_intreq_state;
	bool m_vcd_pixel_clock;
	bool m_pending_fma_stream_change;
	bool m_fmv_decoder_enabled;
	bool m_fmv_playback_active;
	bool m_fmv_update_latched;
	bool m_fmv_show_on_next_frame;

	uint8_t m_fma_stream;
	uint8_t m_fmv_stream;
	uint8_t m_fma_dspa;
	uint8_t m_mpeg_ram_enable_count;

	uint16_t m_fma_command;
	uint16_t m_fma_status;
	uint16_t m_fma_ivec;
	uint16_t m_fma_isr;
	uint16_t m_fma_ier;
	uint16_t m_fma_dclkl_latch;
	uint16_t m_fma_audio_header[2];

	uint16_t m_fmv_syscmd;
	uint16_t m_fmv_vidcmd;
	uint16_t m_fmv_sys_scr;
	uint16_t m_fmv_ivec;
	uint16_t m_fmv_isr;
	uint16_t m_fmv_ier;
	uint16_t m_fmv_timer_compare;
	uint16_t m_fmv_frame_rate;
	uint16_t m_fmv_decoder_command;
	uint16_t m_fmv_vdi_command;
	uint16_t m_fmv_image_width;
	uint16_t m_fmv_image_height;
	uint16_t m_fmv_image_rate;
	uint16_t m_fmv_display_width;
	uint16_t m_fmv_display_height;
	uint16_t m_fmv_picture_rate;
	uint16_t m_fmv_decoded_timestamp;
	uint16_t m_fmv_pictures_in_fifo;
	uint16_t m_fmv_video_ctrl[10];

	uint32_t m_fma_dclk;
	uint32_t m_fmv_dclk;
	uint32_t m_fmv_frame_count;
	uint32_t m_timer_divider;
};

DECLARE_DEVICE_TYPE(CDI_DVC, cdidvc_device)

#endif // MAME_MACHINE_CDIDVC_H
