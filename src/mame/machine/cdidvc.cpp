// license:GPL-3.0-only
// copyright-holders:Andre Zeps, vibecodekun
/******************************************************************************

    Philips CD-i Digital Video Cartridge (VMPEG)

    See cdidvc.h for an overview and for the address decode.

    The register file, interrupt behaviour, program-stream demultiplexer and
    DMA handshake below are a direct port of the CDi_MiSTer VMPEG core by
    Andre Zeps (https://github.com/MiSTer-devel/CDi_MiSTer: rtl/vmpeg.sv,
    rtl/mpeg/mpeg_demuxer.sv) and the accompanying notes in doc/dvc.md.
    Comments quoting register semantics are from that work, which is
    licensed under the GNU General Public License version 3.

*******************************************************************************/

#include "emu.h"
#include "machine/cdidvc.h"

#include "screen.h"

#define LOG_REGS    (1 << 1)
#define LOG_CMD     (1 << 2)
#define LOG_IRQ     (1 << 3)
#define LOG_DMA     (1 << 4)
#define LOG_STREAM  (1 << 5)

// Set to e.g. (LOG_CMD | LOG_IRQ | LOG_DMA) and define LOG_OUTPUT_FUNC as
// osd_printf_info to trace the driver's conversation with the cartridge.
#define VERBOSE     (0)
#include "logmacro.h"

DEFINE_DEVICE_TYPE(CDI_DVC, cdidvc_device, "cdidvc", "CD-i Digital Video Cartridge")

cdidvc_device::cdidvc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, CDI_DVC, tag, owner, clock)
	, device_sound_interface(mconfig, *this)
	, device_video_interface(mconfig, *this)
	, m_intreq_callback(*this)
	, m_memory_space(*this, ":maincpu", AS_PROGRAM)
	, m_scc(*this, ":maincpu")
{
}

//**************************************************************************
//  device_t implementation
//**************************************************************************

void cdidvc_device::device_resolve_objects()
{
	m_intreq_callback.resolve_safe();
}

void cdidvc_device::device_start()
{
	m_mpeg_ram = std::make_unique<uint16_t[]>(0x80000 / 2);
	std::fill_n(m_mpeg_ram.get(), 0x80000 / 2, 0);

	// The cartridge has 512KB of MPEG working RAM; give each decoder a share
	// of it as its input buffer so that overflow behaves like the hardware.
	m_video.set_buffer_limit(320 * 1024);
	m_audio.set_buffer_limit(64 * 1024);

	m_audio_ring.assign(65536, 0);

	m_stream = stream_alloc(0, 2, 44100);

	// FMA/FMV decoder clocks both run at 45 kHz.
	m_dclk_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(cdidvc_device::dclk_tick), this));
	m_dclk_timer->adjust(attotime::never);

	// The picture handed to the MCD212 is swapped over between fields, so that
	// a field is always scanned out from one picture rather than from whatever
	// the decoder happened to be holding as the beam went past.
	screen().register_vblank_callback(vblank_state_delegate(&cdidvc_device::vblank_update, this));

	save_pointer(NAME(m_mpeg_ram), 0x80000 / 2);

	save_item(NAME(m_fma_command));
	save_item(NAME(m_fma_status));
	save_item(NAME(m_fma_isr));
	save_item(NAME(m_fma_ier));
	save_item(NAME(m_fma_ivec));
	save_item(NAME(m_fma_dclk));
	save_item(NAME(m_fma_dclkl_latch));
	save_item(NAME(m_fma_stream));
	save_item(NAME(m_fma_dspa));
	save_item(NAME(m_fma_dsp_enable));
	save_item(NAME(m_fma_stream_change_pending));
	save_item(NAME(m_fma_audio_header));

	save_item(NAME(m_fmv_isr));
	save_item(NAME(m_fmv_ier));
	save_item(NAME(m_fmv_ivec));
	save_item(NAME(m_fmv_dclk));
	save_item(NAME(m_fmv_syscmd));
	save_item(NAME(m_fmv_vidcmd));
	save_item(NAME(m_fmv_sysscr));
	save_item(NAME(m_fmv_dec_cmd));
	save_item(NAME(m_fmv_vdi_cmd));
	save_item(NAME(m_fmv_frame_rate));
	save_item(NAME(m_fmv_timer_compare));
	save_item(NAME(m_timer_count));
	save_item(NAME(m_fmv_stream));
	save_item(NAME(m_fmv_dsp_enable));
	save_item(NAME(m_fmv_playback_active));
	save_item(NAME(m_fmv_decoder_active));
	save_item(NAME(m_fmv_show));
	save_item(NAME(m_fmv_slow_motion));

	save_item(NAME(m_fmv_dclk_start_video));
	save_item(NAME(m_fmv_dclk_start_video_latched));
	save_item(NAME(m_fmv_dclk_pause_video));
	save_item(NAME(m_fmv_dclk_pause_video_latched));

	save_item(NAME(m_regs_update_latch));
	save_item(NAME(m_regs_update_scroll));

	save_item(NAME(m_ctrl_x_offset));
	save_item(NAME(m_ctrl_y_offset));
	save_item(NAME(m_ctrl_x_active));
	save_item(NAME(m_ctrl_y_active));
	save_item(NAME(m_ctrl_x_display));
	save_item(NAME(m_ctrl_y_display));
	save_item(NAME(m_ctrl_window_width));
	save_item(NAME(m_ctrl_window_height));
	save_item(NAME(m_ctrl_decoder_offset_x));
	save_item(NAME(m_ctrl_decoder_offset_y));

	save_item(NAME(m_latched_display_x));
	save_item(NAME(m_latched_display_y));
	save_item(NAME(m_latched_window_x));
	save_item(NAME(m_latched_window_y));
	save_item(NAME(m_latched_window_w));
	save_item(NAME(m_latched_window_h));

	save_item(NAME(m_image_width));
	save_item(NAME(m_image_height));
	save_item(NAME(m_image_rt));

	save_item(NAME(m_mpeg_ram_unlock_count));
	save_item(NAME(m_mpeg_ram_enabled));

	save_item(NAME(m_video_sequence.valid));
	save_item(NAME(m_video_sequence.width));
	save_item(NAME(m_video_sequence.height));
	save_item(NAME(m_video_sequence.frame_rate_index));
	save_item(NAME(m_video_sequence.intra_quant));
	save_item(NAME(m_video_sequence.non_intra_quant));

	save_item(NAME(m_fma_status_decoding));
	save_item(NAME(m_fmv_single_step_pending));
	save_item(NAME(m_video_es_bytes));
	save_item(NAME(m_audio_es_bytes));

	// Picture pacing and the display side.
	save_item(NAME(m_field_count));
	save_item(NAME(m_display_ticks));
	save_item(NAME(m_next_picture_tick));
	save_item(NAME(m_picture_pending));
	save_item(NAME(m_seen_first_picture));
	save_item(NAME(m_video_underflow));
	save_item(NAME(m_display_width));
	save_item(NAME(m_display_height));
	save_item(NAME(m_decoder_timecode));
	save_item(NAME(m_display_timecode));
	save_item(NAME(m_display_video_status));

	save_item(NAME(m_picture_width));
	save_item(NAME(m_picture_height));
	save_item(NAME(m_picture_updated));
	save_item(NAME(m_shown_width));
	save_item(NAME(m_shown_height));
	save_item(NAME(m_shown_enabled));
	save_item(NAME(m_shown_window_active));
	save_item(NAME(m_shown_display_x));
	save_item(NAME(m_shown_display_y));
	save_item(NAME(m_shown_window_x));
	save_item(NAME(m_shown_window_y));
	save_item(NAME(m_shown_window_w));
	save_item(NAME(m_shown_window_h));
	save_item(NAME(m_shown_picture_save));
	save_item(NAME(m_shown_picture_pixels));

	// The audio ring is a fixed 65536 entries from device_start onwards, so it
	// can be registered where it lies.
	save_pointer(NAME(m_audio_ring.data()), m_audio_ring.size());
	save_item(NAME(m_audio_write));
	save_item(NAME(m_audio_read));
	save_item(NAME(m_audio_primed));
	save_item(NAME(m_audio_prime_wait));
	save_item(NAME(m_fma_last_audio_dclk));

	// The decoders' own state. See device_pre_save()/device_post_load() for how
	// it gets in and out of these.
	save_item(NAME(m_video_state.buffer_size));
	save_item(NAME(m_video_state.bit_pos));
	save_item(NAME(m_video_state.buffer));
	save_item(NAME(m_video_state.sequence_ended));
	save_item(NAME(m_video_state.picture_type));
	save_item(NAME(m_video_state.temporal_reference));
	save_item(NAME(m_video_state.frames_saved));
	save_item(NAME(m_video_state.past_index));
	save_item(NAME(m_video_state.future_index));
	save_item(NAME(m_video_state.work_index));
	save_item(NAME(m_video_state.display_index));
	save_item(NAME(m_video_state.frame_valid));
	save_item(NAME(m_video_state.frame_picture_type));
	save_item(NAME(m_video_state.frame_temporal_reference));
	save_item(NAME(m_video_state.frame_y));
	save_item(NAME(m_video_state.frame_cb));
	save_item(NAME(m_video_state.frame_cr));

	save_item(NAME(m_audio_state.buffer_size));
	save_item(NAME(m_audio_state.bit_pos));
	save_item(NAME(m_audio_state.buffer));
	save_item(NAME(m_audio_state.underflow));
	save_item(NAME(m_audio_state.frame_decoded));
	save_item(NAME(m_audio_state.has_header));
	save_item(NAME(m_audio_state.frame_header));
	save_item(NAME(m_audio_state.version));
	save_item(NAME(m_audio_state.layer));
	save_item(NAME(m_audio_state.bitrate_index));
	save_item(NAME(m_audio_state.samplerate_index));
	save_item(NAME(m_audio_state.mode));
	save_item(NAME(m_audio_state.bound));
	save_item(NAME(m_audio_state.sample_rate));
	save_item(NAME(m_audio_state.bit_rate));
	save_item(NAME(m_audio_state.channels));
	save_item(NAME(m_audio_state.samples_per_frame));
	save_item(NAME(m_audio_state.next_frame_size));
	save_item(NAME(m_audio_state.v));
	save_item(NAME(m_audio_state.v_pos));

	save_item(NAME(m_video_demux_state.state));
	save_item(NAME(m_video_demux_state.packet_body));
	save_item(NAME(m_video_demux_state.length_decreasing));
	save_item(NAME(m_video_demux_state.dts_present));
	save_item(NAME(m_video_demux_state.packet_length));
	save_item(NAME(m_video_demux_state.scr));
	save_item(NAME(m_video_demux_state.scr_temp));
	save_item(NAME(m_video_demux_state.pts));
	save_item(NAME(m_video_demux_state.pts_temp));
	save_item(NAME(m_video_demux_state.dts));
	save_item(NAME(m_video_demux_state.dts_temp));
	save_item(NAME(m_video_demux_state.scr_updated));
	save_item(NAME(m_video_demux_state.pts_updated));
	save_item(NAME(m_video_demux_state.dts_updated));
	save_item(NAME(m_video_demux_state.program_end));

	save_item(NAME(m_audio_demux_state.state));
	save_item(NAME(m_audio_demux_state.packet_body));
	save_item(NAME(m_audio_demux_state.length_decreasing));
	save_item(NAME(m_audio_demux_state.dts_present));
	save_item(NAME(m_audio_demux_state.packet_length));
	save_item(NAME(m_audio_demux_state.scr));
	save_item(NAME(m_audio_demux_state.scr_temp));
	save_item(NAME(m_audio_demux_state.pts));
	save_item(NAME(m_audio_demux_state.pts_temp));
	save_item(NAME(m_audio_demux_state.dts));
	save_item(NAME(m_audio_demux_state.dts_temp));
	save_item(NAME(m_audio_demux_state.scr_updated));
	save_item(NAME(m_audio_demux_state.pts_updated));
	save_item(NAME(m_audio_demux_state.dts_updated));
	save_item(NAME(m_audio_demux_state.program_end));

	save_item(NAME(m_fma_playback_start_dclk));
	save_item(NAME(m_fma_playback_start_valid));

	save_item(NAME(m_dma_pending));
	save_item(NAME(m_dma_for_fma));
	save_item(NAME(m_vcd_pixel_clock));
	save_item(NAME(m_intreq));
}

void cdidvc_device::device_reset()
{
	m_fma_command = 0;
	m_fma_status = 0;
	m_fma_isr = 0;
	m_fma_ier = 0;
	m_fma_ivec = 0;
	m_fma_dclk = 0;
	m_fma_dclkl_latch = 0;
	m_fma_stream = 0;
	m_fma_dspa = 0;
	m_fma_dsp_enable = false;
	m_fma_stream_change_pending = false;
	m_fma_status_decoding = false;
	m_fma_playback_start_dclk = 0;
	m_fma_playback_start_valid = false;
	m_fma_audio_header = 0;

	m_fmv_isr = 0;
	m_fmv_ier = 0;
	m_fmv_ivec = 0;
	m_fmv_dclk = 0;
	m_fmv_syscmd = 0;
	m_fmv_vidcmd = 0;
	m_fmv_sysscr = 0;
	m_fmv_dec_cmd = 0;
	m_fmv_vdi_cmd = 0;
	m_fmv_frame_rate = 0;
	m_fmv_timer_compare = 56 - 1;
	m_timer_count = 0;
	m_fmv_stream = 0;
	m_fmv_dsp_enable = false;
	m_fmv_playback_active = false;
	m_fmv_decoder_active = false;
	m_fmv_show = false;
	m_fmv_slow_motion = 0;
	m_fmv_single_step_pending = false;

	m_fmv_dclk_start_video = 0;
	m_fmv_dclk_start_video_latched = false;
	m_fmv_dclk_pause_video = 0;
	m_fmv_dclk_pause_video_latched = false;

	m_regs_update_latch = false;
	m_regs_update_scroll = false;

	m_ctrl_x_offset = m_ctrl_y_offset = 0;
	m_ctrl_x_active = m_ctrl_y_active = 0;
	m_ctrl_x_display = m_ctrl_y_display = 0;
	m_ctrl_window_width = m_ctrl_window_height = 0;
	m_ctrl_decoder_offset_x = m_ctrl_decoder_offset_y = 0;

	m_latched_display_x = m_latched_display_y = 0;
	m_latched_window_x = m_latched_window_y = 0;
	m_latched_window_w = m_latched_window_h = 0;

	m_image_width = m_image_height = m_image_rt = 0;

	// The MPEG working RAM is hidden again after a reset.
	m_mpeg_ram_unlock_count = 0;
	m_mpeg_ram_enabled = false;

	m_video_demux.reset();
	m_audio_demux.reset();
	m_dma_pending = false;
	m_dma_for_fma = false;
	m_vcd_pixel_clock = false;

	m_video_es_bytes = 0;
	m_audio_es_bytes = 0;

	m_video.reset();
	m_audio.reset();
	m_video_sequence = mpeg1_video_decoder::sequence_state();
	m_display_ticks = 0;
	m_next_picture_tick = 0;
	m_picture_pending = false;
	m_seen_first_picture = false;
	m_video_underflow = false;
	m_display_width = m_display_height = 0;
	m_decoder_timecode = m_display_timecode = 0;
	m_display_video_status = 0;
	m_picture.clear();
	m_picture_width = m_picture_height = 0;
	m_picture_updated = false;
	m_shown_picture.clear();
	m_shown_width = m_shown_height = 0;
	m_shown_picture_valid = false;
	m_shown_enabled = false;
	m_shown_window_active = false;
	m_shown_display_x = m_shown_display_y = 0;
	m_shown_window_x = m_shown_window_y = 0;
	m_shown_window_w = m_shown_window_h = 0;
	std::fill(m_audio_ring.begin(), m_audio_ring.end(), 0);
	m_audio_write = m_audio_read = 0;
	m_audio_primed = false;
	m_audio_prime_wait = 0;
	m_fma_last_audio_dclk = 0;

	m_intreq = false;
	m_intreq_callback(0);

	m_dclk_timer->adjust(attotime::from_hz(45000), 0, attotime::from_hz(45000));
}

void cdidvc_device::device_pre_save()
{
	// The sequence parameters are normally latched as each picture is shown;
	// refresh them here so a state written between two pictures still carries
	// the current ones.
	m_video.get_sequence_state(m_video_sequence);

	m_video.get_decoder_state(m_video_state);
	m_audio.get_decoder_state(m_audio_state);
	m_video_demux.get_state(m_video_demux_state);
	m_audio_demux.get_state(m_audio_demux_state);

	// The picture on screen. Anything larger than the cap is left out and the
	// display picks up again from the next decoded picture.
	//
	// An invalidated picture is written out as no picture at all. m_shown_picture
	// is not cleared when the decoder is switched off - the buffer is recycled
	// rather than freed - so the pixel count is the only thing carrying
	// m_shown_picture_valid across a state, and saving the pixels of a picture
	// that is no longer being shown is what brought the previous clip's last
	// frame back on every load. See device_post_load().
	m_shown_picture_pixels = m_shown_picture_valid
		? uint32_t(std::min(m_shown_picture.size(), MAX_SHOWN_PIXELS))
		: 0;
	if (m_shown_picture_pixels != 0)
		std::copy_n(m_shown_picture.begin(), m_shown_picture_pixels, std::begin(m_shown_picture_save));

	// Blank whatever the last save left behind the live pixels, so that two runs
	// showing the same thing write the same bytes. Rewind stores the difference
	// between consecutive states, and run-ahead compares them outright.
	std::fill(std::begin(m_shown_picture_save) + m_shown_picture_pixels,
		std::end(m_shown_picture_save), 0u);
}

void cdidvc_device::device_post_load()
{
	// A restored stream resumes part way through a clip, long after the
	// sequence header that described it went by, so hand the parameters back
	// to the decoder rather than have it discard pictures indefinitely.
	//
	// This has to come first. set_sequence_state() reallocates and clears the
	// frame store whenever the picture size changes, which would throw away the
	// reference pictures that set_decoder_state() is about to restore.
	m_video.set_sequence_state(m_video_sequence);

	m_video.set_decoder_state(m_video_state);
	m_audio.set_decoder_state(m_audio_state);
	m_video_demux.set_state(m_video_demux_state);
	m_audio_demux.set_state(m_audio_demux_state);

	// Put the shown picture back, so the very first field after a load is drawn
	// from the same pixels the saving run was drawing from.
	//
	// The buffer is sized from m_shown_width/m_shown_height whether or not there
	// are any pixels to put in it. vblank_update() swaps this buffer with the
	// decoder's back buffer and swaps the dimensions along with it, so a buffer
	// that disagrees with its own dimensions ends up handed to the decoder - and
	// the decoder reallocates only when the picture size changes, so it would
	// then quietly drop every later picture and the display would freeze.
	const size_t shown_pixels = size_t(std::max(m_shown_width, 0)) * size_t(std::max(m_shown_height, 0));
	m_shown_picture.assign(shown_pixels, 0);
	std::copy_n(m_shown_picture_save, std::min<size_t>(m_shown_picture_pixels, shown_pixels),
		m_shown_picture.begin());

	// m_shown_picture_valid is deliberately not in the save state - adding an
	// item would change the state size and orphan every state written before
	// it - so it travels as the pixel count instead: device_pre_save() writes no
	// picture when the latched frame has been invalidated, and there being no
	// picture to put back is what says so here.
	m_shown_picture_valid = m_shown_picture_pixels != 0;

	// The back buffer has to come back allocated to match m_picture_width and
	// m_picture_height. Its allocation is keyed off those dimensions rather
	// than off the buffer itself, so leaving it empty while they say otherwise
	// makes every later picture get dropped instead of shown, and the display
	// freezes on whatever was restored here.
	if (m_picture_width > 0 && m_picture_height > 0)
		m_picture.assign(size_t(m_picture_width) * m_picture_height, 0);
	else
		m_picture.clear();

	// Its contents only matter while m_picture_updated is set, and it then
	// holds the decoder's current display frame, so rebuild it from there
	// rather than carry a second picture's worth of save state.
	if (m_picture_updated)
	{
		if (!m_picture.empty() && m_video.has_display_frame())
			m_video.display_frame_rgb(m_picture.data(), m_picture_width);
		else
			m_picture_updated = false;
	}

}

//**************************************************************************
//  Interrupts
//**************************************************************************

void cdidvc_device::update_interrupts()
{
	const bool fma = (m_fma_isr & m_fma_ier) != 0;
	const bool fmv = (m_fmv_isr & m_fmv_ier) != 0;
	const bool state = fma || fmv;

	if (state != m_intreq)
	{
		m_intreq = state;
		LOGMASKED(LOG_IRQ, "%s: DVC interrupt %s (FMA ISR %04x IER %04x, FMV ISR %04x IER %04x)\n",
			machine().describe_context(), state ? "asserted" : "cleared",
			m_fma_isr, m_fma_ier, m_fmv_isr, m_fmv_ier);
		m_intreq_callback(state ? 1 : 0);
	}
}

uint8_t cdidvc_device::intack_r()
{
	// FMA wins the vector when both subsystems are asserting.  The FMV vector
	// register holds the level in bits 10..3 rather than the low byte.
	if ((m_fma_isr & m_fma_ier) != 0)
		return m_fma_ivec & 0xff;

	return (m_fmv_ivec >> 3) & 0xff;
}

//**************************************************************************
//  45 kHz decoder clock and timer interrupt
//**************************************************************************

TIMER_CALLBACK_MEMBER(cdidvc_device::dclk_tick)
{
	m_fma_dclk++;
	m_fmv_dclk++;
	m_display_ticks++;

	// Playback start/pause are scheduled a fixed number of DCLK ticks ahead.
	if (m_fmv_dclk_start_video_latched && m_fmv_dclk_start_video == m_fma_dclk)
	{
		m_fmv_dclk_start_video_latched = false;
		m_fmv_playback_active = true;
	}

	if (m_fmv_dclk_pause_video_latched && m_fmv_dclk_pause_video == m_fma_dclk)
	{
		m_fmv_dclk_pause_video_latched = false;
		m_fmv_isr |= FMV_INT_PAI;
	}

	// The driver programs TCNT for a nominal 10ms tick, but the counter runs
	// at 90 kHz / 16, so the achievable period is 100.446... Hz.  fmvdrv is
	// written around that (its SCR increment of 896 assumes it), so the
	// compare below reproduces the hardware divider exactly.
	if (m_timer_count >= (uint32_t(m_fmv_timer_compare) << 3) + 7)
	{
		m_timer_count = 0;
		m_fmv_isr |= FMV_INT_TIM;
		m_fma_isr |= FMA_INT_POLL;
	}
	else
	{
		m_timer_count++;
	}

	if (m_fma_playback_start_valid && int64_t(m_fma_dclk) >= m_fma_playback_start_dclk)
	{
		m_fma_playback_start_valid = false;
		m_fma_dsp_enable = true;
		m_fma_last_audio_dclk = m_fma_dclk;
	}

	// A DMA request that arrived before the 68070 channel was armed is
	// retried here; on hardware the device simply keeps REQ asserted.
	if (m_dma_pending)
		try_dma();

	// Field rate flag. The driver polls this in the interrupt status register
	// while releasing the decoder, so it has to keep ticking regardless of
	// whether anything is playing.
	const uint32_t field_ticks = m_pal ? (45000 / 50) : (45000 / 60);
	bool vsync = false;
	if (++m_field_count >= field_ticks)
	{
		m_field_count = 0;
		vsync = true;
		m_fmv_isr |= FMV_INT_VSYNC;
	}

	// Run the decoders. Video is paced off the stream's own frame period;
	// audio is drained as fast as data arrives, into the output ring.
	const bool picture_shown = advance_video();

	// A pending register update is taken either at vertical retrace or when a
	// new picture starts being displayed, depending on the scroll bit. Until
	// that happens the previously latched geometry stays in force.
	if (m_regs_update_latch && ((m_regs_update_scroll && vsync) || (!m_regs_update_scroll && picture_shown)))
	{
		m_regs_update_latch = false;

		m_latched_display_x = m_ctrl_x_display;
		m_latched_display_y = m_ctrl_y_display;
		m_latched_window_x = m_ctrl_decoder_offset_x;
		m_latched_window_y = m_ctrl_decoder_offset_y;
		m_latched_window_w = m_ctrl_window_width;
		m_latched_window_h = m_ctrl_window_height;

		// The real hardware always raises DCL alongside VCUP.
		m_fmv_isr |= FMV_INT_VCUP | FMV_INT_DCL;
	}

	if ((m_fma_dclk & 0x1f) == 0)
		advance_audio();

	update_interrupts();
}

//**************************************************************************
//  DMA
//**************************************************************************

void cdidvc_device::request_dma(bool for_fma)
{
	m_dma_pending = true;
	m_dma_for_fma = for_fma;
	try_dma();
}

void cdidvc_device::try_dma()
{
	// The cartridge sits on DMA channel 2 of the 68070 (channel index 1);
	// the CDIC owns channel 1.
	auto &channel = m_scc->dma().channel[1];

	const uint32_t count = channel.transfer_counter;
	if (!count)
		return;

	// Memory-to-device is the only direction that makes sense here.
	if (channel.operation_control & OCR_D)
	{
		LOGMASKED(LOG_DMA, "%s: DVC DMA requested in device-to-memory direction, ignoring\n",
			machine().describe_context());
		m_dma_pending = false;
		return;
	}

	const uint32_t start = channel.memory_address_counter;

	LOGMASKED(LOG_DMA, "%s: DVC DMA %s: %04x words from %06x\n", machine().describe_context(),
		m_dma_for_fma ? "FMA" : "FMV", count, start);

	for (uint32_t index = 0; index < count; index++)
	{
		const uint16_t word = m_memory_space->read_word(start + index * 2);
		feed_byte(word >> 8, m_dma_for_fma);
		feed_byte(word & 0xff, m_dma_for_fma);
	}

	channel.memory_address_counter += count * 2;
	channel.transfer_counter = 0;

	// Transfer complete: the device drops its request.  For FMA the command
	// register's DMA bit is cleared to acknowledge.
	m_dma_pending = false;
	if (m_dma_for_fma)
		m_fma_command &= ~0x8000;
	else
		m_fmv_syscmd &= ~SYSCMD_DMA;
	m_dma_for_fma = false;
}

void cdidvc_device::feed_byte(uint8_t data, bool for_fma)
{
	if (for_fma)
	{
		if (m_audio_demux.feed(data, m_fma_stream))
		{
			m_audio_es_bytes++;
			m_audio.write(&data, 1);
		}

		// Playback is scheduled for the moment the decoder clock catches up
		// with the first presentation time stamp of the stream.
		if (m_audio_demux.pts_updated() && !m_fma_playback_start_valid && !m_fma_dsp_enable)
		{
			m_fma_playback_start_valid = true;
			m_fma_playback_start_dclk = int64_t(m_fma_dclk)
				+ (m_audio_demux.presentation_timestamp() >> 1)
				- (m_audio_demux.system_clock_reference() >> 1);
		}

		if (m_audio_demux.program_end())
		{
			m_fma_isr |= FMA_INT_EOI;
			m_fma_status |= 0x01;
		}
	}
	else
	{
		if (m_video_demux.feed(data, m_fmv_stream))
		{
			m_video_es_bytes++;
			m_video.write(&data, 1);
			m_video_underflow = false;
		}

		if (m_video_demux.dts_updated())
			m_fmv_vdi_cmd |= 1 << 14;

		if (m_video_demux.program_end())
			m_fmv_isr |= FMV_INT_EII;
	}
}

//**************************************************************************
//  Decoding
//**************************************************************************

uint32_t cdidvc_device::audio_ring_used() const
{
	const uint32_t capacity = uint32_t(m_audio_ring.size());
	return (m_audio_write >= m_audio_read)
		? (m_audio_write - m_audio_read)
		: (capacity - m_audio_read + m_audio_write);
}

uint32_t cdidvc_device::audio_ring_free() const
{
	// One slot is always left unused so that full and empty stay distinct.
	return uint32_t(m_audio_ring.size()) - audio_ring_used() - 1;
}

bool cdidvc_device::advance_video()
{
	if (!m_fmv_dsp_enable)
		return false;

	// Decode ahead of display so that the picture FIFO count the driver polls
	// reflects work already done, as it would on hardware.
	if (!m_picture_pending)
	{
		m_picture_pending = m_video.decode();
		if (m_picture_pending)
			m_decoder_timecode = m_video.decoded_timecode();

		// FMV_IMG follows the decoder, not the display.  In the RTL it is loaded
		// from decoder_width/height on fmv_event_frame_decoded, and fmvdrv
		// depends on that lead: its interrupt handler compares FMV_IMG against
		// the size it last published, and on a change hands the new one to the
		// title and signals it.  Updating FMV_IMG only when a picture starts
		// display delays that by the whole of the decoder's head start - half a
		// second at the beginning of a clip - so the title spends that long
		// working from the previous clip's dimensions.  Monty Python's Invasion
		// from the Planet Skyron shows it plainly: the title positions each clip
		// from the size the driver reports, so it centred a 368x240 clip as if
		// it were the 128x96 one before it.
		if (m_picture_pending)
		{
			m_image_width = uint16_t(m_video.width());
			m_image_height = uint16_t(m_video.height());
			m_image_rt = m_video.frame_rate_index();
		}
	}

	if (!m_picture_pending)
	{
		// A clip is terminated by a sequence end code - the FMV driver appends
		// one itself when the data runs out - and the decoder only reaches it
		// once the last picture has already been decoded and displayed.  So at
		// the end of every clip the end code turns up on a pass that produces
		// no picture, and announcing it only alongside one, as the display path
		// below does, means the driver never hears that the clip finished: it
		// waits for the end-of-sequence interrupt for ever while the last frame
		// stays on screen.
		if (m_video.sequence_ended())
		{
			m_video.clear_sequence_ended();
			m_fmv_isr |= FMV_INT_ESI | FMV_INT_EOD;
		}

		// NDAT reports the moment the video buffer runs dry, not the state of
		// being dry.  Raising it on every one of the 45 kHz ticks the buffer
		// stays empty - which once a clip has ended is for ever - buries the
		// 68070 under some 45000 interrupts a second, and the driver never gets
		// far enough out of its handler to tear the clip down.  The latch
		// re-arms in feed_byte() as soon as data arrives again.
		if (m_fmv_playback_active && m_video.buffered_bytes() == 0 && !m_video_underflow)
		{
			m_video_underflow = true;
			m_fmv_isr |= FMV_INT_NDAT;
		}
		return false;
	}

	// Hold the picture until its display time arrives.
	if (m_fmv_playback_active && !m_fmv_single_step_pending)
	{
		const uint16_t period = m_video.frame_period_90khz();
		// The pacing counter runs at 45 kHz, half the 90 kHz timestamp clock.
		// VMPEG repeats each picture slow_motion + 1 times (1x through 1/8x).
		const uint32_t ticks = (period ? (period / 2) : 1875) * (m_fmv_slow_motion + 1);

		if (!m_seen_first_picture)
		{
			m_seen_first_picture = true;
			m_next_picture_tick = m_display_ticks + ticks;
		}
		else if (int32_t(m_display_ticks - m_next_picture_tick) < 0)
		{
			return false;
		}
		else
		{
			m_next_picture_tick += ticks;

			// Whenever the decoder has fallen more than a frame behind - the
			// stream stalled, or a save state put the machine somewhere else -
			// picking the schedule up from now is the only sane thing to do.
			// Working off the accumulated debt instead would spit out every
			// buffered picture at the tick rate until it caught up, which on
			// screen is a burst of skipped video.
			if (int32_t(m_display_ticks - m_next_picture_tick) >= 0)
				m_next_picture_tick = m_display_ticks + ticks;
		}
	}
	else if (!m_fmv_single_step_pending)
	{
		return false;
	}

	// The direct single-picture command is used for isolated scan pictures.
	// Normal MPEG playback delays I/P pictures until the following anchor,
	// but a scan may supply only one picture before flushing the decoder.
	if (m_fmv_single_step_pending && (m_fmv_dec_cmd & 0xff00) == 0x2200)
		m_video.display_decoded_frame();
	m_picture_pending = false;
	// Decoding an anchor does not necessarily produce an output picture yet.
	// In scan mode fmvdrv stops on PIC/GOP, so signalling here before the
	// reordered picture exists makes it flush the only anchor and seek again.
	// Keep a step request pending across missing-reference pictures as well.
	if (!m_video.has_display_frame())
		return false;
	m_fmv_single_step_pending = false;

	if (m_video.has_display_frame())
	{
		const int width = m_video.width();
		const int height = m_video.height();

		if (width != m_picture_width || height != m_picture_height)
		{
			m_picture_width = width;
			m_picture_height = height;
			m_picture.assign(size_t(width) * height, 0);
		}

		if (!m_picture.empty())
		{
			m_video.display_frame_rgb(m_picture.data(), width);
			m_picture_updated = true;
		}

		m_video.get_sequence_state(m_video_sequence);

		m_display_width = uint16_t(width);
		m_display_height = uint16_t(height);
		m_display_timecode = m_video.display_timecode();
		// fmvdrv uses SYS_VSR's picture type to resync its clock at an I
		// picture, and its temporal reference to offset the GOP timecode.
		// Report the displayed picture, including when anchors are reordered.
		m_display_video_status = (m_video.display_temporal_reference() << 2) | m_video.display_picture_type();
	}

	// A picture starting display is what the driver counts on for pacing.
	m_fmv_isr |= FMV_INT_PIC;

	if (m_video.display_picture_type() == 1)
		m_fmv_isr |= FMV_INT_GOP | FMV_INT_SEQ;

	if (m_video.sequence_ended())
	{
		m_video.clear_sequence_ended();
		m_fmv_isr |= FMV_INT_ESI | FMV_INT_EOD;
	}

	return true;
}

void cdidvc_device::advance_audio()
{
	if (!m_fma_dsp_enable)
		return;

	for (;;)
	{
		// Stop before the output ring fills rather than after.  Decoding a
		// frame there is nowhere to put simply throws it away, and since the
		// driver hands over audio in bursts far faster than real time, that
		// silently drops most of a clip.  Leaving the frame in the decoder's
		// input buffer is also what the hardware does: it is the MPEG buffer
		// that backs up, and the driver watches its level.
		if (audio_ring_free() < mpeg1_audio_decoder::max_samples_per_frame * 2)
			break;

		const int pairs = m_audio.decode();
		if (pairs <= 0)
			break;

		if (!m_fma_status_decoding)
		{
			m_fma_status_decoding = true;
			m_fma_status |= FMA_INT_DEC;
			m_fma_isr |= FMA_INT_DEC;
		}

		m_fma_audio_header = m_audio.frame_header();
		m_fma_status |= FMA_INT_UPD;
		m_fma_isr |= FMA_INT_UPD;

		if (m_fma_stream_change_pending)
		{
			m_fma_isr |= FMA_INT_CSU;
			m_fma_stream_change_pending = false;
		}

		const int16_t *const src = m_audio.samples();
		const uint32_t capacity = uint32_t(m_audio_ring.size());
		for (int i = 0; i < pairs * 2; i++)
		{
			const uint32_t next = (m_audio_write + 1) % capacity;
			if (next == m_audio_read)
				break;              // ring full; drop the rest of the frame
			m_audio_ring[m_audio_write] = src[i];
			m_audio_write = next;
		}
	}

	// Anything still sitting in the decoder's input buffer, or already decoded
	// and waiting in the output ring, is audio the cartridge has yet to play.
	if (m_audio.buffered_bytes() != 0 || audio_ring_used() >= 2)
		m_fma_last_audio_dclk = m_fma_dclk;

	if (m_audio.underflowed())
	{
		m_audio.clear_underflow();

		// The decoder raises underflow whenever it does not hold a whole frame,
		// which is the normal state between transfers: the driver feeds the
		// cartridge just in time and a partial frame is nearly always left over.
		// A real underflow is the cartridge's own MPEG buffer running dry, so
		// that is what has to be tested here.
		//
		// Testing buffered_bytes() alone is not enough, because the compressed
		// data is consumed here as soon as it arrives and the samples then wait
		// in m_audio_ring - an emulator artifact with no hardware counterpart -
		// rather than in the buffer the driver is watching.  The input buffer
		// therefore holds nothing at all every time the byte count happens to
		// land on a frame boundary, which for a 192 kbit/s clip is every 400
		// frames, or once every 10.4 seconds.  Reporting FMA_INT_UNF there makes
		// madriv stop the stream and the FMV player tear down the whole clip,
		// including video that is still buffered and playing perfectly well.
		//
		// So the buffer counts as dry only when there is neither a frame left to
		// decode nor any decoded audio left to play, and when it has stayed that
		// way for longer than the driver's own refill interval.  fmvdrv hands
		// over a sector roughly every 100 ms and can be as much as 330 ms late,
		// while the cartridge's real 64KB audio buffer would take some 2.7
		// seconds to empty, so half a second sits comfortably between the two.
		static constexpr uint32_t DRY_TICKS = 45000 / 2;

		if (m_fma_status_decoding && (m_fma_dclk - m_fma_last_audio_dclk) >= DRY_TICKS)
		{
			m_fma_status |= FMA_INT_UNF;
			m_fma_isr |= FMA_INT_UNF;
			m_fma_status &= ~FMA_INT_DEC;
			m_fma_status_decoding = false;
			m_fma_dsp_enable = false;
			m_fma_playback_start_valid = false;
		}
	}
}

void cdidvc_device::sound_stream_update(sound_stream &stream, std::vector<read_stream_view> const &inputs, std::vector<write_stream_view> &outputs)
{
	auto &left = outputs[0];
	auto &right = outputs[1];

	const uint32_t capacity = uint32_t(m_audio_ring.size());

	// Rebuild the cushion once the cartridge has stopped and the ring has run
	// out, so the next clip starts with a full one.
	if (!m_fma_dsp_enable && audio_ring_used() < 2)
	{
		m_audio_primed = false;
		m_audio_prime_wait = 0;
	}

	if (!m_audio_primed)
	{
		if (audio_ring_used() >= 2)
			m_audio_prime_wait += uint32_t(left.samples());

		if (audio_ring_used() < k_audio_prime_samples && m_audio_prime_wait < k_audio_prime_timeout)
		{
			left.fill(0);
			right.fill(0);
			return;
		}
		m_audio_primed = true;
	}

	for (int i = 0; i < left.samples(); i++)
	{
		// Consume whole stereo pairs only.  Taking half a pair when the ring
		// happens to hold an odd number of samples puts every later sample in
		// the wrong channel, and nothing ever puts them back.
		if (audio_ring_used() < 2)
		{
			left.put(i, 0);
			right.put(i, 0);
			continue;
		}

		const int16_t l = m_audio_ring[m_audio_read];
		const int16_t r = m_audio_ring[(m_audio_read + 1) % capacity];
		m_audio_read = (m_audio_read + 2) % capacity;

		left.put_int(i, l, 32768);
		right.put_int(i, r, 32768);
	}
}

//**************************************************************************
//  Register reads
//**************************************************************************

uint16_t cdidvc_device::regs_r(offs_t offset, uint16_t mem_mask)
{
	// Only address bits 15..1 are decoded, so the file mirrors every 64KB.
	const offs_t reg = offset & 0x7fff;
	uint16_t result = 0;

	switch (reg)
	{
	// ---- FMA ----
	case 0x1800: result = m_fma_command; break;
	case 0x1801: result = 0x0200 | m_fma_status; break;
	case 0x1802: result = 0x0007; break;
	case 0x1803: result = 0x0900; break;
	case 0x1804: result = m_fma_stream; break;    // wanted stream id
	case 0x1805: result = m_fma_stream; break;    // current stream id
	case 0x1806: result = m_fma_ivec; break;
	case 0x1807: result = 0x0042; break;

	case 0x1808:
		// Reading the high word latches the low word so the pair is coherent.
		result = m_fma_dclk >> 16;
		if (!machine().side_effects_disabled())
			m_fma_dclkl_latch = m_fma_dclk & 0xffff;
		break;

	case 0x1809: result = m_fma_dclkl_latch; break;
	case 0x180a: result = m_fma_audio_header >> 16; break;
	case 0x180b: result = m_fma_audio_header & 0xffff; break;
	case 0x180c: result = m_fma_dsp_enable ? 1 : 0; break;

	case 0x180d:
		result = m_fma_isr;
		if (!machine().side_effects_disabled())
		{
			m_fma_isr = 0;
			update_interrupts();
		}
		break;

	case 0x180e: result = m_fma_ier; break;
	case 0x1812: result = 0x0004; break;          // HF2 flag of the DSP56001

	// ---- FMV ----
	case 0x2001: result = m_image_width; break;
	case 0x2002: result = m_image_height; break;
	case 0x2003: result = m_image_rt; break;
	case 0x2004: result = m_display_timecode >> 16; break;
	case 0x2005: result = m_display_timecode & 0xffff; break;

	case 0x2029: result = m_display_width; break;
	case 0x202a: result = m_display_height; break;
	case 0x202b: result = m_video.frame_rate_index(); break;
	case 0x202c: result = m_decoder_timecode >> 16; break;
	case 0x202d: result = m_decoder_timecode & 0xffff; break;
	case 0x202e: result = m_display_video_status; break;
	case 0x202f: result = m_video.buffer_full() ? 0 : 0x2000; break;  // SYS_STS

	case 0x2030: result = m_fmv_ier; break;

	case 0x2031:
		result = m_fmv_isr;
		if (!machine().side_effects_disabled())
		{
			m_fmv_isr = 0;
			update_interrupts();
		}
		break;

	case 0x2032: result = m_fmv_timer_compare; break;

	case 0x2036: result = m_ctrl_y_offset; break;
	case 0x2037: result = m_ctrl_x_offset; break;
	case 0x2038: result = m_ctrl_y_active; break;
	case 0x2039: result = m_ctrl_x_active; break;
	case 0x203a: result = m_ctrl_y_display; break;
	case 0x203b: result = m_ctrl_x_display; break;
	case 0x203c: result = m_ctrl_window_height; break;
	case 0x203d: result = m_ctrl_window_width; break;
	case 0x203e: result = m_ctrl_decoder_offset_y; break;
	case 0x203f: result = m_ctrl_decoder_offset_x; break;

	case 0x2044: result = m_fmv_dec_cmd; break;
	case 0x2046: result = m_fmv_vdi_cmd; break;
	case 0x2049: result = 0; break;               // GEN_VID_BUF
	case 0x204c: result = (m_fmv_dclk >> 6) & 0xffff; break;  // GEN_SYSCR
	case 0x204e: result = 0; break;               // reads 0 on real hardware
	case 0x204f: result = 1; break;

	case 0x2050:
		// GEN_DEC_TIM1: the DTS as the CPU sees it, at 703.125 Hz resolution.
		result = uint16_t((m_video_demux.decoding_timestamp() >> 7) & 0x7fff);
		break;

	case 0x2052: result = uint16_t(m_video.pictures_buffered()); break;
	case 0x2054: result = m_video.frame_period_90khz(); break;
	case 0x2055: result = m_pal ? 0x0708 : 0x05dc; break;  // display rate
	case 0x2056: result = m_fmv_frame_rate; break;
	case 0x2060: result = m_fmv_syscmd; break;
	case 0x2061: result = m_fmv_vidcmd; break;
	case 0x2062: result = m_fmv_stream; break;
	case 0x206e: result = m_fmv_ivec; break;
	case 0x2073: result = 0; break;               // MMU base pointer, ignored

	default:
		LOGMASKED(LOG_REGS, "%s: DVC read of unhandled register %04x & %04x\n",
			machine().describe_context(), reg * 2, mem_mask);
		break;
	}

	LOGMASKED(LOG_REGS, "%s: DVC read %06x = %04x & %04x\n", machine().describe_context(),
		0xe00000 + reg * 2, result, mem_mask);

	return result;
}

//**************************************************************************
//  Register writes
//**************************************************************************

void cdidvc_device::write_fma_cmd(uint16_t data)
{
	LOGMASKED(LOG_CMD, "%s: FMA CMD = %04x\n", machine().describe_context(), data);

	m_fma_command = data;

	if (BIT(data, 15))
	{
		// Green book 8.2.4.3.3: a transfer clears the underflow status.
		m_fma_status &= ~FMA_INT_UNF;
		request_dma(true);
	}

	if (BIT(data, 0))
	{
		// Stop
		m_fma_dsp_enable = false;
		m_fma_status = 0;
		m_fma_status_decoding = false;
		m_fma_playback_start_valid = false;
		m_fma_last_audio_dclk = m_fma_dclk;
		m_audio_demux.reset();
		m_audio.reset();
	}
}

void cdidvc_device::write_syscmd(uint16_t data)
{
	LOGMASKED(LOG_CMD, "%s: FMV SYSCMD = %04x\n", machine().describe_context(), data);

	m_fmv_syscmd = data;

	if (data & SYSCMD_PLAY)
	{
		m_fmv_dclk_start_video = m_fma_dclk + 1000;
		m_fmv_dclk_start_video_latched = true;
		m_fmv_dec_cmd |= 0x42;
		m_fmv_slow_motion = data & 0x07;
		m_fmv_decoder_active = true;
		m_seen_first_picture = false;
		m_video_underflow = false;
	}

	if (data & SYSCMD_PAUSE)
	{
		m_fmv_playback_active = false;
		m_fmv_dclk_start_video_latched = false;
		m_fmv_dclk_pause_video = m_fma_dclk + 100;
		m_fmv_dclk_pause_video_latched = true;
	}

	if (data & SYSCMD_CONTINUE)
	{
		m_fmv_dclk_start_video = m_fma_dclk + 1000;
		m_fmv_dclk_start_video_latched = true;
		m_fmv_slow_motion = data & 0x07;
	}

	if (data & SYSCMD_STEP)
	{
		m_fmv_dclk_start_video_latched = false;
		m_fmv_single_step_pending = true;
	}

	if (data & SYSCMD_STOP)
	{
		m_fmv_single_step_pending = false;
		m_fmv_playback_active = false;
		m_fmv_dclk_start_video_latched = false;
		m_fmv_decoder_active = false;
	}

	if (data & SYSCMD_CLEAR_FIFO)
	{
		m_fmv_single_step_pending = false;
		m_fmv_playback_active = false;
		m_fmv_dclk_start_video_latched = false;
		m_video_demux.reset();
		// FIFO clear invalidates compressed data and prediction references,
		// but VMPEG keeps its persistent sequence configuration until decoder
		// off. A seek need not supply another sequence header (The Firm).
		m_video.clear_fifo();
		m_picture_pending = false;
		m_seen_first_picture = false;
		m_video_underflow = false;
		// The decoder is bounced off and straight back on again.
		m_fmv_dsp_enable = true;
	}

	if (data & SYSCMD_DEC_ON)
	{
		m_fmv_dsp_enable = true;
		LOGMASKED(LOG_CMD, "%s: FMV decoder on\n", machine().describe_context());
	}

	if (data & SYSCMD_DEC_OFF)
	{
		m_fmv_single_step_pending = false;
		m_fmv_dsp_enable = false;
		m_fmv_playback_active = false;
		m_fmv_dclk_start_video_latched = false;
		m_fmv_decoder_active = false;
		m_image_width = 0;
		m_image_height = 0;
		m_image_rt = 0;
		m_video_demux.reset();
		m_video.reset();
		m_picture_pending = false;
		m_seen_first_picture = false;
		m_video_underflow = false;
		// Decoder off is what the RTL calls resetting persistent storage, and
		// it invalidates the latched display frame. Drop the back buffer's
		// pending update along with it, or a picture decoded just before the
		// teardown would be swapped in afterwards and count as this clip's.
		m_shown_picture_valid = false;
		m_picture_updated = false;
		LOGMASKED(LOG_CMD, "%s: FMV decoder off\n", machine().describe_context());
	}

	if (data & SYSCMD_DMA)
		request_dma(false);
}

void cdidvc_device::write_vidcmd(uint16_t data)
{
	LOGMASKED(LOG_CMD, "%s: FMV VIDCMD = %04x\n", machine().describe_context(), data);

	m_fmv_vidcmd = data;

	if (BIT(data, 3))       // RegsUpd
	{
		m_regs_update_latch = true;
		m_regs_update_scroll = BIT(data, 2);
	}

	if (BIT(data, 8))       // Hide
		m_fmv_show = false;

	if (BIT(data, 9))       // Show
		m_fmv_show = true;

	if (BIT(data, 10))      // Show on next picture change
		m_fmv_show = true;
}

void cdidvc_device::regs_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	const offs_t reg = offset & 0x7fff;

	LOGMASKED(LOG_REGS, "%s: DVC write %06x = %04x & %04x\n", machine().describe_context(),
		0xe00000 + reg * 2, data, mem_mask);

	// The MPEG working RAM only becomes visible once the driver has done
	// enough register traffic to prove it is really there.  Hardware uses a
	// six-bit counter; until it wraps, the OS-9 RAM crawler must not be able
	// to see the memory.
	if (m_mpeg_ram_unlock_count == 0x3f)
	{
		if (!m_mpeg_ram_enabled)
			LOGMASKED(LOG_REGS, "%s: DVC MPEG RAM unlocked\n", machine().describe_context());
		m_mpeg_ram_enabled = true;
	}
	m_mpeg_ram_unlock_count = (m_mpeg_ram_unlock_count + 1) & 0x3f;

	// VMPEG pixel clock select lives at e01xxx.
	if ((reg & 0x7f00) == 0x1000)
	{
		m_vcd_pixel_clock = BIT(data, 0);
		return;
	}

	switch (reg)
	{
	// ---- FMA ----
	case 0x1800: write_fma_cmd(data); break;

	case 0x1802:
		// The VMPEG ROM writes an immediate 7 here right after setting the
		// FMA stream number; purpose unknown, no effect modelled.
		break;

	case 0x1804:
		m_fma_stream = data & 0x0f;
		m_fma_stream_change_pending = true;
		break;

	case 0x1806: m_fma_ivec = data; break;
	case 0x180c: break;                             // FMA RUN

	case 0x180e:
		m_fma_ier = data;
		update_interrupts();
		break;

	case 0x1811: m_fma_dspa = data & 0xff; break;
	case 0x1812: break;                             // FMA DSPD

	// ---- FMV ----
	case 0x2001: m_image_width = data; break;
	case 0x2002: m_image_height = data; break;
	case 0x2003: m_image_rt = data; break;

	case 0x2030:
		m_fmv_ier = data;
		update_interrupts();
		break;

	case 0x2031: break;                             // ISR is read-to-clear
	case 0x2032: m_fmv_timer_compare = data; break;

	case 0x2036: m_ctrl_y_offset = data; break;
	case 0x2037: m_ctrl_x_offset = data; break;
	case 0x2038: m_ctrl_y_active = data; break;
	case 0x2039: m_ctrl_x_active = data; break;
	case 0x203a: m_ctrl_y_display = data; break;
	case 0x203b: m_ctrl_x_display = data; break;
	case 0x203c: m_ctrl_window_height = data; break;
	case 0x203d: m_ctrl_window_width = data; break;
	case 0x203e: m_ctrl_decoder_offset_y = data; break;
	case 0x203f: m_ctrl_decoder_offset_x = data; break;

	case 0x2044:
		m_fmv_dec_cmd = data;
		// GEN_DEC_CMD 22xx requests a single picture (also used by fmvdrv
		// for fast forward/reverse). As in VMPEG, an active pause command
		// inhibits this request. Complete it through the normal picture
		// display path so that PIC and pending clip updates are delivered.
		if ((data & 0xff00) == 0x2200 && !(m_fmv_syscmd & SYSCMD_PAUSE))
			m_fmv_single_step_pending = true;
		break;
	case 0x2046: m_fmv_vdi_cmd = data; break;
	case 0x2049: break;                             // GEN_VID_BUF

	case 0x204c:
		// GEN_SYSCR: only bits 21..6 of the 45 kHz clock are CPU-writable.
		m_fmv_dclk = (m_fmv_dclk & ~(0xffffu << 6)) | (uint32_t(data) << 6);
		break;

	case 0x2056: m_fmv_frame_rate = data; break;
	case 0x2057: m_timer_count = 0; break;          // TRLD
	case 0x2060: write_syscmd(data); break;
	case 0x2061: write_vidcmd(data); break;
	case 0x2062: m_fmv_stream = data & 0x0f; break;
	case 0x2063: m_fmv_sysscr = data; break;
	case 0x206e: m_fmv_ivec = data; break;

	case 0x206f:
		// XFER: programmed-I/O alternative to DMA.  Each write pushes one
		// 16-bit word of program stream into the video path.
		feed_byte(data >> 8, false);
		feed_byte(data & 0xff, false);
		break;

	default:
		LOGMASKED(LOG_REGS, "%s: DVC write to unhandled register %04x = %04x\n",
			machine().describe_context(), reg * 2, data);
		break;
	}
}

//**************************************************************************
//  MPEG working RAM
//**************************************************************************

uint16_t cdidvc_device::mpeg_ram_r(offs_t offset, uint16_t mem_mask)
{
	return m_mpeg_ram[offset & 0x3ffff];
}

void cdidvc_device::mpeg_ram_w(offs_t offset, uint16_t data, uint16_t mem_mask)
{
	COMBINE_DATA(&m_mpeg_ram[offset & 0x3ffff]);
}

//**************************************************************************
//  Video output
//**************************************************************************

void cdidvc_device::vblank_update(screen_device &screen, bool state)
{
	// Only on the way into vertical blanking: what is latched here is what the
	// whole of the next field will be drawn from.
	if (!state)
		return;

	// The picture buffers are swapped rather than copied; the decoder always
	// writes a complete picture, so whatever is left in the buffer it gets
	// back does not matter.
	if (m_picture_updated)
	{
		m_picture_updated = false;
		m_picture.swap(m_shown_picture);
		std::swap(m_picture_width, m_shown_width);
		std::swap(m_picture_height, m_shown_height);
		m_shown_picture_valid = true;
	}

	// The geometry has to move at the same moment as the picture, or a field
	// ends up drawn half at the old offset and half at the new one.
	m_shown_enabled = m_fmv_show && m_fmv_dsp_enable;
	m_shown_display_x = m_latched_display_x;
	m_shown_display_y = m_latched_display_y;
	m_shown_window_x = m_latched_window_x;
	m_shown_window_y = m_latched_window_y;

	// What the cartridge displays is the decoded picture, taken from the DECOFF
	// corner and placed at SCRPOS.  DECWIN does not narrow it further.
	//
	// DECWIN is not something titles program directly.  fmvdrv keeps a display
	// window in its own state, hands it to the cartridge whenever it updates the
	// registers, and - this is the important part - only ever *shrinks* it: the
	// interrupt handler clips the window to the picture size every time a clip
	// decodes at a new size (fmvdrv PSWndw/e541ca, and the clip at e5439a), and
	// nothing in the driver ever grows it again.  So once a title has played one
	// small clip, the driver's window stays that small for every clip after it.
	//
	// Monty Python's Invasion from the Planet Skyron shows what that costs.  It
	// alternates 128x96 inserts with 368x240 full-screen clips and never calls
	// mv_window() itself, so the window is always the *previous* clip's size:
	// the register trace has the driver programming DECWIN 128x96 for the
	// 368x240 "INTERMISSION" card (which then appears as a corner of itself),
	// and DECWIN 268x175 for a 128x96 card (which then sits in a black surround
	// covering the artwork behind it).  Both are the same bug seen from the two
	// sides.  Since fmvdrv is Philips' own driver and the title is Philips' own
	// content, hardware cannot be taking DECWIN as a hard crop the way the
	// MiSTer frameplayer does - its shrink-only bookkeeping would break every
	// title that changes picture size, not just this one.
	//
	// The window still bounds where the cartridge is keyed in over the CD-i
	// planes, which is what Mutant Rampage needs for the border it draws around
	// its MPEG window; that title programs DECWIN to exactly its picture size,
	// so deriving the window from the picture leaves it unchanged.
	const int avail_w = m_shown_width - int(m_latched_window_x);
	const int avail_h = m_shown_height - int(m_latched_window_y);
	m_shown_window_w = uint16_t(std::max(avail_w, 0));
	m_shown_window_h = uint16_t(std::max(avail_h, 0));

	m_shown_window_active = m_fmv_dsp_enable && m_shown_window_w != 0 && m_shown_window_h != 0;
}

bool cdidvc_device::video_enabled() const
{
	return m_shown_enabled;
}

bool cdidvc_device::window_covers(int x, int y) const
{
	if (!m_shown_window_active)
		return false;

	const int px = x - int(m_shown_display_x);
	const int py = y - int(m_shown_display_y);

	return px >= 0 && py >= 0 && px < int(m_shown_window_w) && py < int(m_shown_window_h);
}

bool cdidvc_device::get_pixel(int x, int y, uint32_t &result) const
{
	if (!m_shown_enabled || !m_shown_picture_valid || m_shown_picture.empty())
		return false;

	// The picture is placed at the display offset latched at the last register
	// update, and runs from the decode offset to the end of the picture.
	const int px = x - int(m_shown_display_x);
	const int py = y - int(m_shown_display_y);

	if (px < 0 || py < 0)
		return false;

	if (px >= int(m_shown_window_w) || py >= int(m_shown_window_h))
		return false;

	// Within the window, the source pixel is offset by the decode offset.
	const int sx = px + int(m_shown_window_x);
	const int sy = py + int(m_shown_window_y);

	if (sx < 0 || sy < 0 || sx >= m_shown_width || sy >= m_shown_height)
		return false;

	result = m_shown_picture[size_t(sy) * m_shown_width + sx];
	return true;
}
