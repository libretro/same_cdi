// license:GPL-3.0-only
// copyright-holders:Andre Zeps, vibecodekun
/******************************************************************************

    Philips CD-i Digital Video Cartridge (VMPEG)
    --------------------------------------------

    The DVC is an expansion cartridge that adds MPEG-1 audio and video decoding
    to a CD-i player.  Several chipsets exist; this device models the VMPEG
    variant (as fitted to the 22ER9141 cartridge and built into the CD-i 490),
    which pairs an MCD251 video decoder with a DSP56001 doing audio in software.

    The register-level behaviour modelled here was derived from the
    documentation and RTL of the CDi_MiSTer project by Andre Zeps, which
    reverse-engineered the cartridge in detail:

        https://github.com/MiSTer-devel/CDi_MiSTer   (doc/dvc.md, rtl/vmpeg.sv)

    CDi_MiSTer is licensed under the GNU General Public License version 3,
    and this device is a port of it, so cdidvc.cpp/.h and mpeg1demux.cpp/.h
    are distributed under the same license.

    Address decode (as seen from the 68070):

        d00000-dfffff   1MB of extra system RAM, always visible
        e00000-e3ffff   VMPEG registers (only address bits 15..1 are decoded,
                        so the register file mirrors every 64KB)
        e40000-e7ffff   128KB driver ROM (fmvdrv/madriv OS-9 modules), mirrored
        e80000-efffff   512KB MPEG working RAM, hidden until unlocked

    The MPEG RAM is deliberately not visible after reset.  The OS-9 memory
    crawler would otherwise find it and hand it out as general system RAM,
    which breaks the decoder.  Hardware gates it behind a counter that unlocks
    once the driver has performed 64 register writes.

    TODO:

    - Picture display is paced off the sequence's own frame rate rather than
      the decoding time stamps, so a stream whose audio and video drift apart
      will not be resynchronised.

*******************************************************************************/

#ifndef MAME_MACHINE_CDIDVC_H
#define MAME_MACHINE_CDIDVC_H

#pragma once

#include "machine/scc68070.h"
#include "machine/mpeg1demux.h"
#include "machine/mpeg1video.h"
#include "machine/mpeg1audio.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class cdidvc_device : public device_t, public device_sound_interface, public device_video_interface
{
public:
	cdidvc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	auto intreq_callback() { return m_intreq_callback.bind(); }

	// e00000-e3ffff register window
	uint16_t regs_r(offs_t offset, uint16_t mem_mask = ~0);
	void regs_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);

	// e80000-efffff MPEG working RAM
	uint16_t mpeg_ram_r(offs_t offset, uint16_t mem_mask = ~0);
	void mpeg_ram_w(offs_t offset, uint16_t data, uint16_t mem_mask = ~0);
	bool mpeg_ram_enabled() const { return m_mpeg_ram_enabled; }

	uint8_t intack_r();
	bool intreq() const { return m_intreq; }

	void set_pal(bool pal) { m_pal = pal; }

	// Video output to the MCD212 external-video (backdrop) path.
	//
	// window_covers() reports whether the cartridge's display window is open
	// over a pixel, which is a separate question from whether there is a
	// picture to put there: the window stays open across the gap between clips,
	// and the CD-i plane behind it stays keyed away, so what shows is black
	// rather than the plane.  get_pixel() returns false when there is nothing
	// to display, and the caller should then fall back to black.
	//
	// The window is the decoded picture, taken from the DECOFF corner; DECWIN
	// does not narrow it, because fmvdrv leaves it holding the *previous*
	// clip's size.  vblank_update() has the detail.
	bool video_enabled() const;
	bool window_covers(int x, int y) const;
	bool get_pixel(int x, int y, uint32_t &result) const;

protected:
	virtual void device_resolve_objects() override;
	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void sound_stream_update(sound_stream &stream, std::vector<read_stream_view> const &inputs, std::vector<write_stream_view> &outputs) override;

	virtual void device_pre_save() override;
	virtual void device_post_load() override;

	TIMER_CALLBACK_MEMBER(dclk_tick);

private:
	void vblank_update(screen_device &screen, bool state);

	// FMV interrupt status/enable bits.  Bit numbering follows the packed
	// struct in the MiSTer RTL, where the first-declared field is the MSB.
	enum : uint16_t
	{
		FMV_INT_SEQ   = 1 << 0,   // sequence header decoded
		FMV_INT_GOP   = 1 << 1,   // group-of-pictures decoded
		FMV_INT_PIC   = 1 << 2,   // picture started display
		FMV_INT_EOD   = 1 << 3,   // end of data
		FMV_INT_RFB   = 1 << 4,   // request for bits
		FMV_INT_NDAT  = 1 << 5,   // no data / underflow
		FMV_INT_OVF   = 1 << 6,   // overflow
		FMV_INT_DCL   = 1 << 7,
		FMV_INT_TIM   = 1 << 8,   // timer
		FMV_INT_ESI   = 1 << 9,   // end sequence indicator
		FMV_INT_EII   = 1 << 10,  // end ISO indicator
		FMV_INT_VSYNC = 1 << 11,
		FMV_INT_PAI   = 1 << 12,  // pause
		FMV_INT_VCUP  = 1 << 13,  // video clip update
		FMV_INT_ERDD  = 1 << 14,
		FMV_INT_ERDV  = 1 << 15
	};

	// FMA interrupt status/enable bits
	enum : uint16_t
	{
		FMA_INT_EOI  = 1 << 0,  // ISO end detected
		FMA_INT_CSU  = 1 << 1,  // audio stream changed
		FMA_INT_UPD  = 1 << 2,  // frame header updated
		FMA_INT_UNF  = 1 << 3,  // underflow
		FMA_INT_DEC  = 1 << 4,  // decoding started
		FMA_INT_ERR  = 1 << 5,
		FMA_INT_POLL = 1 << 8
	};

	// FMV SYSCMD (e040c0) command bits
	enum : uint16_t
	{
		SYSCMD_PLAY       = 1 << 3,
		SYSCMD_PAUSE      = 1 << 4,
		SYSCMD_CONTINUE   = 1 << 5,
		SYSCMD_STEP       = 1 << 6,
		SYSCMD_STOP       = 1 << 7,
		SYSCMD_CLEAR_FIFO = 1 << 8,
		SYSCMD_SEARCH_GOP = 1 << 10,
		SYSCMD_DEC_ON     = 1 << 12,
		SYSCMD_DEC_OFF    = 1 << 13,
		SYSCMD_DMA        = 1 << 15
	};

	void update_interrupts();
	uint32_t audio_ring_used() const;
	uint32_t audio_ring_free() const;
	void request_dma(bool for_fma);
	void try_dma();
	void feed_byte(uint8_t data, bool for_fma);
	bool advance_video();
	void advance_audio();
	void write_syscmd(uint16_t data);
	void write_vidcmd(uint16_t data);
	void write_fma_cmd(uint16_t data);

	devcb_write_line m_intreq_callback;

	required_address_space m_memory_space;
	required_device<scc68070_device> m_scc;

	emu_timer *m_dclk_timer = nullptr;

	bool m_pal = true;

	// ---- FMA (audio) registers ----
	uint16_t m_fma_command = 0;
	uint8_t  m_fma_status = 0;
	uint16_t m_fma_isr = 0;
	uint16_t m_fma_ier = 0;
	uint16_t m_fma_ivec = 0;
	uint32_t m_fma_dclk = 0;
	uint16_t m_fma_dclkl_latch = 0;
	uint8_t  m_fma_stream = 0;
	uint8_t  m_fma_dspa = 0;
	bool     m_fma_dsp_enable = false;
	bool     m_fma_stream_change_pending = false;
	bool     m_fma_status_decoding = false;
	// The audio subsystem starts decoding when the stream's own clock reaches
	// the first presentation time stamp, rather than on an explicit command.
	int64_t  m_fma_playback_start_dclk = 0;
	bool     m_fma_playback_start_valid = false;
	uint32_t m_fma_audio_header = 0;

	// ---- FMV (video) registers ----
	uint16_t m_fmv_isr = 0;
	uint16_t m_fmv_ier = 0;
	uint16_t m_fmv_ivec = 0;
	uint32_t m_fmv_dclk = 0;
	uint16_t m_fmv_syscmd = 0;
	uint16_t m_fmv_vidcmd = 0;
	uint16_t m_fmv_sysscr = 0;
	uint16_t m_fmv_dec_cmd = 0;
	uint16_t m_fmv_vdi_cmd = 0;
	uint16_t m_fmv_frame_rate = 0;
	uint16_t m_fmv_timer_compare = 56 - 1;
	uint32_t m_timer_count = 0;
	uint8_t  m_fmv_stream = 0;
	bool     m_fmv_dsp_enable = false;
	bool     m_fmv_playback_active = false;
	bool     m_fmv_decoder_active = false;
	bool     m_fmv_show = false;
	bool     m_fmv_single_step_pending = false;
	uint8_t  m_fmv_slow_motion = 0;

	uint32_t m_fmv_dclk_start_video = 0;
	bool     m_fmv_dclk_start_video_latched = false;
	uint32_t m_fmv_dclk_pause_video = 0;
	bool     m_fmv_dclk_pause_video_latched = false;

	bool m_regs_update_latch = false;
	bool m_regs_update_scroll = false;

	// mv_org()/mv_pos()/mv_window() geometry, written by the driver
	uint16_t m_ctrl_x_offset = 0, m_ctrl_y_offset = 0;
	uint16_t m_ctrl_x_active = 0, m_ctrl_y_active = 0;
	uint16_t m_ctrl_x_display = 0, m_ctrl_y_display = 0;
	uint16_t m_ctrl_window_width = 0, m_ctrl_window_height = 0;
	uint16_t m_ctrl_decoder_offset_x = 0, m_ctrl_decoder_offset_y = 0;

	// Latched at register-update time, these are what the display uses.
	// m_latched_window_w/h are the DECWIN size; the display path does not use
	// it - see vblank_update() for why - but it is latched all the same, since
	// the register still has to read back the way the hardware's does.
	uint16_t m_latched_display_x = 0, m_latched_display_y = 0;
	uint16_t m_latched_window_x = 0, m_latched_window_y = 0;
	uint16_t m_latched_window_w = 0, m_latched_window_h = 0;

	uint16_t m_image_width = 0, m_image_height = 0, m_image_rt = 0;

	// ---- unlock counter for the MPEG working RAM ----
	uint8_t m_mpeg_ram_unlock_count = 0;
	bool    m_mpeg_ram_enabled = false;
	std::unique_ptr<uint16_t[]> m_mpeg_ram;

	// ---- stream plumbing ----
	mpeg1_demuxer m_video_demux{ 0xe0 };
	mpeg1_demuxer m_audio_demux{ 0xc0 };
	bool    m_dma_pending = false;
	bool    m_dma_for_fma = false;
	bool    m_vcd_pixel_clock = false;

	bool m_intreq = false;

	// Counts of elementary-stream bytes handed to each decoder; useful when
	// tracing whether the driver is actually streaming.
	uint64_t m_video_es_bytes = 0;
	uint64_t m_audio_es_bytes = 0;

	// --- decoders ---
	//
	// The decoders are kept free of MAME, so they hand their state over as
	// plain structs that this device owns, registers and copies across in
	// device_pre_save()/device_post_load(). Between them they add rather more
	// than a megabyte to the save state, most of it the two reference pictures
	// and the input FIFOs.
	mpeg1_video_decoder::sequence_state m_video_sequence;
	mpeg1_video_decoder::decoder_state m_video_state;
	mpeg1_audio_decoder::decoder_state m_audio_state;
	mpeg1_demuxer::demux_state m_video_demux_state;
	mpeg1_demuxer::demux_state m_audio_demux_state;
	mpeg1_video_decoder m_video;
	mpeg1_audio_decoder m_audio;

	// Pictures are pulled out of the decoder on the stream's own frame period.
	//
	// The deadline is measured against m_display_ticks, a plain count of 45 kHz
	// ticks, and deliberately not against m_fmv_dclk.  GEN_SYSCR lets the driver
	// write the stream's system clock reference straight into m_fmv_dclk, so
	// that register jumps backwards and forwards with the stream; pacing off it
	// leaves the deadline unreachable and the picture never changes again.
	uint32_t m_field_count = 0;
	uint32_t m_display_ticks = 0;
	uint32_t m_next_picture_tick = 0;
	bool     m_picture_pending = false;
	bool     m_seen_first_picture = false;

	// Latch behind FMV_INT_NDAT, so that the video buffer running dry is
	// reported once per occurrence rather than on every tick it stays empty.
	bool     m_video_underflow = false;

	// Display geometry reported back to the driver.
	uint16_t m_display_width = 0, m_display_height = 0;
	uint32_t m_decoder_timecode = 0, m_display_timecode = 0;
	uint8_t  m_display_video_status = 0;

	// Decoded picture, already converted to RGB for the MCD212 to pick up.
	//
	// The MCD212 is configured for VIDEO_UPDATE_SCANLINE, so it asks for the
	// cartridge's pixels one scanline at a time as the field is scanned out,
	// while the decoder replaces the picture whenever the 45 kHz clock says the
	// next one is due.  Handing get_pixel() the buffer the decoder writes into
	// therefore splices two different pictures - and two different sets of
	// geometry - into one displayed field, which is what shows up as tearing.
	//
	// So the decoder writes into m_picture and everything the display reads
	// lives in the m_shown_* copies, swapped over once per field in
	// vblank_update().
	std::vector<uint32_t> m_picture;
	int m_picture_width = 0, m_picture_height = 0;
	bool m_picture_updated = false;

	std::vector<uint32_t> m_shown_picture;
	int m_shown_width = 0, m_shown_height = 0;

	// Whether the picture on the display side is one this clip put there.
	//
	// This is the frameplayer's latched_frame_valid: the cartridge shows a
	// picture only once one has been latched for display, and switching the
	// decoder off invalidates whatever was latched. Without it the previous
	// clip's last frame stays in the display buffer and is put back on screen
	// the moment the driver re-enables the window for the *next* clip, a couple
	// of fields before that clip's first picture is ready - which is seen as
	// the outgoing clip's final frame, often at the wrong position, flashing up
	// again just before the new one starts.
	//
	// It has to survive a save state, or rewind and run-ahead - which load one
	// every frame - put the stale frame back on screen as fast as this clears
	// it. See device_pre_save() for how it travels.
	bool m_shown_picture_valid = false;

	// The shown picture has to ride along in the save state. It is derived from
	// the decoder's frame store, but not reconstructably so: by the time a
	// state is written the decoder has usually moved on to a newer picture and
	// the one on screen may already have been recycled.
	//
	// The pixel count doubles as m_shown_picture_valid, which has no save item
	// of its own: an invalidated picture is written out as no picture at all.
	//
	// The back buffer m_picture needs no such copy. It only ever matters while
	// m_picture_updated is set, and it then holds exactly the decoder's current
	// display frame, so device_post_load() rebuilds it instead.
	static constexpr size_t MAX_SHOWN_PIXELS = 384 * 288;
	uint32_t m_shown_picture_save[MAX_SHOWN_PIXELS] = { 0 };
	uint32_t m_shown_picture_pixels = 0;
	bool m_shown_enabled = false;
	// The display window outlives the picture: it is open whenever the decoder
	// is powered up and the driver has given it a size, whether or not a clip
	// is currently running through it.
	bool m_shown_window_active = false;
	uint16_t m_shown_display_x = 0, m_shown_display_y = 0;
	uint16_t m_shown_window_x = 0, m_shown_window_y = 0;
	uint16_t m_shown_window_w = 0, m_shown_window_h = 0;

	// Audio output ring, drained by the sound stream.
	//
	// Playback holds off until the ring has k_audio_prime_samples in it.  The
	// driver hands the cartridge audio just in time - it keeps on the order of
	// 120ms of it outstanding - while the sound stream drains a whole update at
	// a time, so with no cushion the ring comes up a few hundred samples short
	// on the trough of every delivery cycle.  The stream then has to insert
	// silence, and those short gaps are what is heard as crackling.  The cushion
	// is rebuilt whenever the cartridge stops and drains, so it costs its own
	// depth in latency once per clip rather than accumulating.
	//
	// The cushion has to be deeper than one delivery burst, or the first burst
	// satisfies it and it buys nothing.  How deep that has to be depends on the
	// title: Mutant Rampage hands over about 95ms at a time, but Monty Python's
	// Invasion from the Planet Skyron opens a clip by delivering 185ms every
	// 280ms - fully caught up on average, but a third of each cycle spent dry -
	// and keeps that up for the first four seconds.  A 150ms cushion is emptied
	// by the trough of every one of those cycles, which is heard as the clicking
	// and stuttering that clears up once the driver settles into a tighter
	// rhythm.  Measured against that title's delivery, 250ms is the point where
	// the ring stops reaching zero at all; 300ms leaves some margin and still
	// costs well under half the ring's 743ms depth.
	//
	// A title that keeps less audio outstanding than the cushion would never
	// reach it and would play nothing at all, so once audio is actually arriving
	// the wait is given up on after this long and playback starts with whatever
	// has accumulated.  The timer only runs while the ring has something in it:
	// before that there is no audio yet and nothing is lost by waiting.
	static constexpr uint32_t k_audio_prime_samples = (44100 * 300 / 1000) * 2; // 300ms, stereo
	static constexpr uint32_t k_audio_prime_timeout = 44100 / 2;               // 500ms
	sound_stream *m_stream = nullptr;
	std::vector<int16_t> m_audio_ring;
	uint32_t m_audio_write = 0, m_audio_read = 0;
	bool m_audio_primed = false;
	uint32_t m_audio_prime_wait = 0;

	// Last time the audio path held anything at all to play, used to tell a
	// real buffer underflow from the gaps between the driver's transfers.
	uint32_t m_fma_last_audio_dclk = 0;
};

DECLARE_DEVICE_TYPE(CDI_DVC, cdidvc_device)

#endif // MAME_MACHINE_CDIDVC_H
