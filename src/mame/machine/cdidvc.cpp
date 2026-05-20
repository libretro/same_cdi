// license:BSD-3-Clause
// copyright-holders:Ryan Holtz

#include "emu.h"
#include "machine/cdidvc.h"

#include <algorithm>


DEFINE_DEVICE_TYPE(CDI_DVC, cdidvc_device, "cdidvc", "CD-i Digital Video Cartridge")


cdidvc_device::cdidvc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, CDI_DVC, tag, owner, clock)
	, m_intreq_callback(*this)
	, m_memory_space(*this, ":maincpu", AS_PROGRAM)
	, m_scc(*this, ":maincpu")
	, m_rom(*this, DEVICE_SELF)
	, m_dclk_timer(nullptr)
	, m_intreq_state(false)
	, m_vcd_pixel_clock(false)
	, m_pending_fma_stream_change(false)
	, m_fmv_decoder_enabled(false)
	, m_fmv_playback_active(false)
	, m_fmv_update_latched(false)
	, m_fmv_show_on_next_frame(false)
	, m_fma_stream(0)
	, m_fmv_stream(0)
	, m_fma_dspa(0)
	, m_mpeg_ram_enable_count(0)
	, m_fma_command(0)
	, m_fma_status(0)
	, m_fma_ivec(0)
	, m_fma_isr(0)
	, m_fma_ier(0)
	, m_fma_dclkl_latch(0)
	, m_fma_audio_header{0, 0}
	, m_fmv_syscmd(0)
	, m_fmv_vidcmd(0)
	, m_fmv_sys_scr(0)
	, m_fmv_ivec(0)
	, m_fmv_isr(0)
	, m_fmv_ier(0)
	, m_fmv_timer_compare(56 - 1)
	, m_fmv_frame_rate(0)
	, m_fmv_decoder_command(0)
	, m_fmv_vdi_command(0)
	, m_fmv_image_width(0)
	, m_fmv_image_height(0)
	, m_fmv_image_rate(0)
	, m_fmv_display_width(352)
	, m_fmv_display_height(288)
	, m_fmv_picture_rate(0)
	, m_fmv_decoded_timestamp(0)
	, m_fmv_pictures_in_fifo(0)
	, m_fmv_video_ctrl{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
	, m_fma_dclk(0)
	, m_fmv_dclk(0)
	, m_fmv_frame_count(0)
	, m_timer_divider(0)
{
}

void cdidvc_device::device_resolve_objects()
{
	m_intreq_callback.resolve_safe();
}

void cdidvc_device::device_start()
{
	m_mpeg_ram = std::make_unique<uint16_t[]>(0x40000);

	m_dclk_timer = timer_alloc(TIMER_DCLK);
	m_dclk_timer->adjust(attotime::from_hz(45'000), 0, attotime::from_hz(45'000));

	save_pointer(NAME(m_mpeg_ram), 0x40000);
	save_item(NAME(m_intreq_state));
	save_item(NAME(m_vcd_pixel_clock));
	save_item(NAME(m_pending_fma_stream_change));
	save_item(NAME(m_fmv_decoder_enabled));
	save_item(NAME(m_fmv_playback_active));
	save_item(NAME(m_fmv_update_latched));
	save_item(NAME(m_fmv_show_on_next_frame));
	save_item(NAME(m_fma_stream));
	save_item(NAME(m_fmv_stream));
	save_item(NAME(m_fma_dspa));
	save_item(NAME(m_mpeg_ram_enable_count));
	save_item(NAME(m_fma_command));
	save_item(NAME(m_fma_status));
	save_item(NAME(m_fma_ivec));
	save_item(NAME(m_fma_isr));
	save_item(NAME(m_fma_ier));
	save_item(NAME(m_fma_dclkl_latch));
	save_item(NAME(m_fma_audio_header));
	save_item(NAME(m_fmv_syscmd));
	save_item(NAME(m_fmv_vidcmd));
	save_item(NAME(m_fmv_sys_scr));
	save_item(NAME(m_fmv_ivec));
	save_item(NAME(m_fmv_isr));
	save_item(NAME(m_fmv_ier));
	save_item(NAME(m_fmv_timer_compare));
	save_item(NAME(m_fmv_frame_rate));
	save_item(NAME(m_fmv_decoder_command));
	save_item(NAME(m_fmv_vdi_command));
	save_item(NAME(m_fmv_image_width));
	save_item(NAME(m_fmv_image_height));
	save_item(NAME(m_fmv_image_rate));
	save_item(NAME(m_fmv_display_width));
	save_item(NAME(m_fmv_display_height));
	save_item(NAME(m_fmv_picture_rate));
	save_item(NAME(m_fmv_decoded_timestamp));
	save_item(NAME(m_fmv_pictures_in_fifo));
	save_item(NAME(m_fmv_video_ctrl));
	save_item(NAME(m_fma_dclk));
	save_item(NAME(m_fmv_dclk));
	save_item(NAME(m_fmv_frame_count));
	save_item(NAME(m_timer_divider));
}

void cdidvc_device::device_reset()
{
	m_intreq_state = false;
	m_vcd_pixel_clock = false;
	m_pending_fma_stream_change = false;
	m_fmv_decoder_enabled = false;
	m_fmv_playback_active = false;
	m_fmv_update_latched = false;
	m_fmv_show_on_next_frame = false;
	m_fma_stream = 0;
	m_fmv_stream = 0;
	m_fma_dspa = 0;
	m_mpeg_ram_enable_count = 0;
	m_fma_command = 0;
	m_fma_status = 0;
	m_fma_ivec = 0;
	m_fma_isr = 0;
	m_fma_ier = 0;
	m_fma_dclkl_latch = 0;
	m_fma_audio_header[0] = 0;
	m_fma_audio_header[1] = 0;
	m_fmv_syscmd = 0;
	m_fmv_vidcmd = 0;
	m_fmv_sys_scr = 0;
	m_fmv_ivec = 0;
	m_fmv_isr = 0;
	m_fmv_ier = 0;
	m_fmv_timer_compare = 56 - 1;
	m_fmv_frame_rate = 0;
	m_fmv_decoder_command = 0;
	m_fmv_vdi_command = 0;
	m_fmv_image_width = 0;
	m_fmv_image_height = 0;
	m_fmv_image_rate = 0;
	m_fmv_display_width = 352;
	m_fmv_display_height = 288;
	m_fmv_picture_rate = 0;
	m_fmv_decoded_timestamp = 0;
	m_fmv_pictures_in_fifo = 0;
	std::fill_n(m_fmv_video_ctrl, 10, 0);
	m_fma_dclk = 0;
	m_fmv_dclk = 0;
	m_fmv_frame_count = 0;
	m_timer_divider = 0;
	update_interrupt_state();
}

void cdidvc_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	if (id == TIMER_DCLK)
		dclk_tick();
}

uint16_t cdidvc_device::read_rom(offs_t byte_offset) const
{
	if (!m_rom.found() || !m_rom.length())
		return 0;

	const size_t mirror_size = std::min<size_t>(m_rom.length(), 0x20000);
	const size_t rom_offset = byte_offset % mirror_size;

	return (uint16_t(m_rom[rom_offset]) << 8) | m_rom[(rom_offset + 1) % mirror_size];
}

uint32_t cdidvc_device::make_timecode() const
{
	const uint32_t frames_per_second = 25;
	const uint32_t frames = m_fmv_frame_count % frames_per_second;
	const uint32_t seconds_total = m_fmv_frame_count / frames_per_second;
	const uint32_t seconds = seconds_total % 60;
	const uint32_t minutes_total = seconds_total / 60;
	const uint32_t minutes = minutes_total % 60;
	const uint32_t hours = (minutes_total / 60) & 0x1f;

	return (seconds << 22) | (frames << 16) | (hours << 6) | minutes;
}

bool cdidvc_device::mpeg_ram_enabled() const
{
	return m_mpeg_ram_enable_count >= 64;
}

void cdidvc_device::trigger_bus_error(offs_t offset, bool read)
{
	if (machine().side_effects_disabled())
		return;

	m_scc->set_buserror_details(0xe80000 | (offset << 1), read, m_scc->get_fc());
	m_scc->set_input_line(M68K_LINE_BUSERROR, ASSERT_LINE);
	m_scc->set_input_line(M68K_LINE_BUSERROR, CLEAR_LINE);
}

void cdidvc_device::update_interrupt_state()
{
	const bool active = ((m_fma_isr & m_fma_ier) != 0) || ((m_fmv_isr & m_fmv_ier) != 0);
	if (active != m_intreq_state)
	{
		m_intreq_state = active;
		m_intreq_callback(active ? ASSERT_LINE : CLEAR_LINE);
	}
}

void cdidvc_device::dclk_tick()
{
	m_fma_dclk++;
	m_fmv_dclk++;

	if ((m_timer_divider >> 3) >= m_fmv_timer_compare)
	{
		m_timer_divider = 0;
		m_fmv_isr |= FMV_INT_TIM;
		m_fma_isr |= FMA_INT_POLL;
		update_interrupt_state();
	}
	else
	{
		m_timer_divider++;
	}
}

void cdidvc_device::complete_dma(int channel)
{
	scc68070_device::dma_channel_t &dma = m_scc->dma().channel[channel];
	dma.memory_address_counter += dma.transfer_counter * 2;
	dma.channel_status |= CSR_COC;
}

void cdidvc_device::process_fma_dma()
{
	scc68070_device::dma_channel_t &dma = m_scc->dma().channel[1];
	const uint32_t start = dma.memory_address_counter;

	m_fma_audio_header[0] = m_memory_space->read_word(start + 0x0c);
	m_fma_audio_header[1] = m_memory_space->read_word(start + 0x0e);

	m_fma_status |= 0x14;
	m_fma_isr |= FMA_INT_DEC | FMA_INT_UPD;
	if (m_pending_fma_stream_change)
	{
		m_fma_isr |= FMA_INT_CSU;
		m_pending_fma_stream_change = false;
	}

	m_fma_command &= 0x7fff;
	complete_dma(1);
	update_interrupt_state();
}

void cdidvc_device::process_fmv_dma()
{
	if (m_fmv_image_width == 0)
		m_fmv_image_width = 352;
	if (m_fmv_image_height == 0)
		m_fmv_image_height = 288;
	if (m_fmv_image_rate == 0)
		m_fmv_image_rate = 3;

	m_fmv_display_width = m_fmv_image_width;
	m_fmv_display_height = m_fmv_image_height;
	m_fmv_picture_rate = 0x0708;
	m_fmv_decoded_timestamp = (m_fmv_decoded_timestamp + 1800) & 0x7fff;
	m_fmv_frame_count++;

	if (m_fmv_pictures_in_fifo < 31)
		m_fmv_pictures_in_fifo++;

	m_fmv_vdi_command |= 0x4000;
	m_fmv_isr |= FMV_INT_PIC;
	if (m_fmv_frame_count == 1)
		m_fmv_isr |= FMV_INT_SEQ | FMV_INT_GOP;
	if (m_fmv_update_latched)
	{
		m_fmv_update_latched = false;
		m_fmv_isr |= FMV_INT_VCUP | FMV_INT_DCL;
	}

	complete_dma(1);
	update_interrupt_state();
}

uint16_t cdidvc_device::regs_r(offs_t offset, uint16_t mem_mask)
{
	const offs_t byte_offset = offset << 1;

	if (byte_offset >= 0x40000)
		return read_rom(byte_offset - 0x40000);

	switch (offset & 0x7fff)
	{
		case 0x1800: return m_fma_command;                         // E03000 FMA CMD
		case 0x1801: return 0x0200 | (m_fma_status & 0x00ff);      // E03002 FMA STAT
		case 0x1802: return 0x0007;                                // E03004
		case 0x1803: return 0x0900;                                // E03006
		case 0x1804: return m_fma_stream;                          // E03008 wanted stream
		case 0x1805: return m_fma_stream;                          // E0300A current stream
		case 0x1806: return m_fma_ivec;                             // E0300C FMA IVEC
		case 0x1807: return 0x0042;                                // E0300E
		case 0x1808: m_fma_dclkl_latch = m_fma_dclk; return m_fma_dclk >> 16; // E03010
		case 0x1809: return m_fma_dclkl_latch;                     // E03012
		case 0x180a: return m_fma_audio_header[0];                 // E03014
		case 0x180b: return m_fma_audio_header[1];                 // E03016
		case 0x180c: return m_fma_status & 0x10 ? 1 : 0;           // E03018 RUN
		case 0x180d:                                                // E0301A FMA ISR
		{
			const uint16_t result = m_fma_isr;
			if (!machine().side_effects_disabled())
			{
				m_fma_isr = 0;
				update_interrupt_state();
			}
			return result;
		}
		case 0x180e: return m_fma_ier;                              // E0301C FMA IER
		case 0x1812: return 0x0004;                                // E03024 DSP HF2

		case 0x2001: return m_fmv_image_width;                     // E04002
		case 0x2002: return m_fmv_image_height;                    // E04004
		case 0x2003: return m_fmv_image_rate;                      // E04006
		case 0x2004: return make_timecode() >> 16;                 // E04008
		case 0x2005: return make_timecode();                       // E0400A
		case 0x2029: return m_fmv_display_width;                   // E04052
		case 0x202a: return m_fmv_display_height;                  // E04054
		case 0x202b: return m_fmv_image_rate;                      // E04056
		case 0x202c: return make_timecode() >> 16;                 // E04058
		case 0x202d: return make_timecode();                       // E0405A
		case 0x202e: return (m_fmv_frame_count & 0xff) << 2;       // E0405C
		case 0x202f: return 0x2000;                                // E0405E SYS_STS
		case 0x2030: return m_fmv_ier;                              // E04060 FMV IER
		case 0x2031:                                                // E04062 FMV ISR
		{
			const uint16_t result = m_fmv_isr;
			if (!machine().side_effects_disabled())
			{
				m_fmv_isr = 0;
				update_interrupt_state();
			}
			return result;
		}
		case 0x2032: return m_fmv_timer_compare;                   // E04064
		case 0x2036: return m_fmv_video_ctrl[0];                   // E0406C VOFF
		case 0x2037: return m_fmv_video_ctrl[1];                   // E0406E HOFF
		case 0x2038: return m_fmv_video_ctrl[2];                   // E04070 VPIX
		case 0x2039: return m_fmv_video_ctrl[3];                   // E04072 HPIX
		case 0x203a: return m_fmv_video_ctrl[4];                   // E04074 SCRPOS Y
		case 0x203b: return m_fmv_video_ctrl[5];                   // E04076 SCRPOS X
		case 0x203c: return m_fmv_video_ctrl[6];                   // E04078 DECWIN H
		case 0x203d: return m_fmv_video_ctrl[7];                   // E0407A DECWIN W
		case 0x203e: return m_fmv_video_ctrl[8];                   // E0407C DECOFF Y
		case 0x203f: return m_fmv_video_ctrl[9];                   // E0407E DECOFF X
		case 0x2044: return m_fmv_decoder_command;                 // E04088 GEN_DEC_CMD
		case 0x2046: return m_fmv_vdi_command;                     // E0408C GEN_VDI_CMD
		case 0x204c: return m_fmv_dclk >> 6;                       // E04098 GEN_SYSCR
		case 0x204e: return 0;                                     // E0409C GEN_SYNC_DIFF
		case 0x204f: return 0xfe96;                                // E0409E GEN_DEC_DELAY
		case 0x2050: return m_fmv_decoded_timestamp;               // E040A0
		case 0x2052: return m_fmv_pictures_in_fifo;                // E040A4
		case 0x2054: return m_fmv_picture_rate;                    // E040A8
		case 0x2055: return 0x0708;                                // E040AA display rate (PAL)
		case 0x2056: return m_fmv_frame_rate;                      // E040AC
		case 0x2060: return m_fmv_syscmd;                          // E040C0
		case 0x2061: return m_fmv_vidcmd;                          // E040C2
		case 0x2062: return m_fmv_stream;                          // E040C4
		case 0x2063: return m_fmv_sys_scr;                         // E040C6
		case 0x206e: return m_fmv_ivec;                             // E040DC
		case 0x2073: return 0;                                     // E040E6
	}

	return 0;
}

void cdidvc_device::regs_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	const offs_t byte_offset = offset << 1;
	const offs_t reg = offset & 0x7fff;

	if (!machine().side_effects_disabled() && byte_offset < 0x40000 && m_mpeg_ram_enable_count != 0xff)
		m_mpeg_ram_enable_count++;

	if (byte_offset >= 0x1000 && byte_offset <= 0x1fff)
	{
		m_vcd_pixel_clock = BIT(data, 0);
		return;
	}

	switch (reg)
	{
		case 0x1800: // E03000 FMA CMD
			COMBINE_DATA(&m_fma_command);
			if (m_fma_command & 0x8000)
				process_fma_dma();
			if (m_fma_command & 0x0001)
			{
				m_fma_status = 0;
				m_fma_isr = 0;
				update_interrupt_state();
			}
			break;

		case 0x1804: // E03008 FMA stream
			m_fma_stream = data & 0x0f;
			m_pending_fma_stream_change = true;
			break;

		case 0x1806: // E0300C FMA IVEC
			COMBINE_DATA(&m_fma_ivec);
			break;

		case 0x180e: // E0301C FMA IER
			COMBINE_DATA(&m_fma_ier);
			update_interrupt_state();
			break;

		case 0x1811: // E03022 FMA DSPA
			m_fma_dspa = data & 0xff;
			break;

		case 0x1812: // E03024 FMA DSPD
			break;

		case 0x2001:
			COMBINE_DATA(&m_fmv_image_width);
			break;
		case 0x2002:
			COMBINE_DATA(&m_fmv_image_height);
			break;
		case 0x2003:
			COMBINE_DATA(&m_fmv_image_rate);
			break;
		case 0x2030: // E04060 FMV IER
			COMBINE_DATA(&m_fmv_ier);
			update_interrupt_state();
			break;
		case 0x2031: // E04062 FMV ISR
			m_fmv_isr &= ~data;
			update_interrupt_state();
			break;
		case 0x2032: // E04064 FMV timer
			COMBINE_DATA(&m_fmv_timer_compare);
			break;
		case 0x2036:
		case 0x2037:
		case 0x2038:
		case 0x2039:
		case 0x203a:
		case 0x203b:
		case 0x203c:
		case 0x203d:
		case 0x203e:
		case 0x203f:
			COMBINE_DATA(&m_fmv_video_ctrl[reg - 0x2036]);
			break;
		case 0x2044:
			COMBINE_DATA(&m_fmv_decoder_command);
			if ((m_fmv_decoder_command & 0xff00) == 0x2200 && m_fmv_playback_active)
				m_fmv_isr |= FMV_INT_PIC;
			update_interrupt_state();
			break;
		case 0x2046:
			COMBINE_DATA(&m_fmv_vdi_command);
			break;
		case 0x204c:
			m_fmv_dclk &= 0xffc0003f;
			m_fmv_dclk |= uint32_t(data & 0xffff) << 6;
			break;
		case 0x2056:
			COMBINE_DATA(&m_fmv_frame_rate);
			break;
		case 0x2057:
			m_timer_divider = 0;
			break;
		case 0x2060: // E040C0 FMV SYSCMD
			COMBINE_DATA(&m_fmv_syscmd);
			if (m_fmv_syscmd & 0x0008)
				m_fmv_playback_active = true;
			if (m_fmv_syscmd & 0x0010)
			{
				m_fmv_playback_active = false;
				m_fmv_isr |= FMV_INT_PAI;
			}
			if (m_fmv_syscmd & 0x0020)
				m_fmv_playback_active = true;
			if (m_fmv_syscmd & 0x0040)
				m_fmv_isr |= FMV_INT_PIC;
			if (m_fmv_syscmd & 0x0080)
				m_fmv_playback_active = false;
			if (m_fmv_syscmd & 0x0100)
			{
				m_fmv_pictures_in_fifo = 0;
				m_fmv_vdi_command = 0;
			}
			if (m_fmv_syscmd & 0x1000)
				m_fmv_decoder_enabled = true;
			if (m_fmv_syscmd & 0x2000)
			{
				m_fmv_decoder_enabled = false;
				m_fmv_playback_active = false;
				m_fmv_image_width = 0;
				m_fmv_image_height = 0;
				m_fmv_image_rate = 0;
			}
			if (m_fmv_syscmd & 0x8000)
				process_fmv_dma();
			update_interrupt_state();
			break;
		case 0x2061: // E040C2 FMV VIDCMD
			COMBINE_DATA(&m_fmv_vidcmd);
			if (m_fmv_vidcmd & 0x0008)
			{
				m_fmv_update_latched = false;
				m_fmv_isr |= FMV_INT_VCUP | FMV_INT_DCL;
			}
			if (m_fmv_vidcmd & 0x0100)
				m_fmv_show_on_next_frame = false;
			if (m_fmv_vidcmd & 0x0600)
				m_fmv_show_on_next_frame = true;
			update_interrupt_state();
			break;
		case 0x2062:
			m_fmv_stream = data & 0x0f;
			break;
		case 0x2063:
			COMBINE_DATA(&m_fmv_sys_scr);
			break;
		case 0x206e:
			COMBINE_DATA(&m_fmv_ivec);
			break;
		case 0x206f:
			break;
	}
}

uint16_t cdidvc_device::mpeg_ram_r(offs_t offset, uint16_t mem_mask)
{
	if (!mpeg_ram_enabled())
	{
		trigger_bus_error(offset, true);
		return 0x00ff;
	}

	return m_mpeg_ram[offset & 0x3ffff];
}

void cdidvc_device::mpeg_ram_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	if (!mpeg_ram_enabled())
	{
		trigger_bus_error(offset, false);
		return;
	}

	COMBINE_DATA(&m_mpeg_ram[offset & 0x3ffff]);
}

uint8_t cdidvc_device::intack_r()
{
	if (m_fma_isr & m_fma_ier)
		return m_fma_ivec & 0xff;
	else if (m_fmv_isr & m_fmv_ier)
		return (m_fmv_ivec >> 3) & 0xff;

	return 0x40;
}
