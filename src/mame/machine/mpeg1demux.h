// license:GPL-3.0-only
// copyright-holders:Andre Zeps, vibecodekun
/******************************************************************************

    ISO/IEC 11172-1 (MPEG-1) program stream demultiplexer

    A byte-at-a-time state machine, ported from the CDi_MiSTer VMPEG core by
    Andre Zeps (https://github.com/MiSTer-devel/CDi_MiSTer,
    rtl/mpeg/mpeg_demuxer.sv), which is licensed under the GNU General Public
    License version 3. The Digital Video Cartridge runs one instance
    for audio and one for video, each fed its own copy of the program stream by
    the driver, and each picking out the elementary stream it cares about.

    Kept free of MAME dependencies so it can be tested against captured discs.

*******************************************************************************/

#ifndef MAME_MACHINE_MPEG1DEMUX_H
#define MAME_MACHINE_MPEG1DEMUX_H

#pragma once

#include <cstdint>

class mpeg1_demuxer
{
public:
	// stream_class is 0xc0 for audio or 0xe0 for video.
	explicit mpeg1_demuxer(uint8_t stream_class = 0xe0) : m_stream_class(stream_class) { }

	void set_stream_class(uint8_t stream_class) { m_stream_class = stream_class; }
	void reset();

	// Feeds one byte. Returns true when that byte is part of the body of a
	// packet belonging to the selected stream, i.e. elementary stream data.
	bool feed(uint8_t data, uint8_t stream_filter);

	// Timestamps carried by the stream, in 90 kHz ticks. The *_updated flags
	// refer to the most recent call to feed().
	int64_t system_clock_reference() const { return m_scr; }
	int64_t presentation_timestamp() const { return m_pts; }
	int64_t decoding_timestamp() const { return m_dts; }

	bool scr_updated() const { return m_scr_updated; }
	bool pts_updated() const { return m_pts_updated; }
	bool dts_updated() const { return m_dts_updated; }
	bool program_end() const { return m_program_end; }

	// The whole of the state machine, as plain data so the owning device can
	// save it. A demuxer restored mid-packet has to come back knowing how much
	// of the packet body is still to come, or it will hand the rest of the
	// packet to the decoder as though it were start codes.
	//
	// The stream class is configuration rather than state and is deliberately
	// left out; the owner sets it when it constructs the demuxer.
	struct demux_state
	{
		uint8_t  state = IDLE;
		uint8_t  packet_body = 0;
		uint8_t  length_decreasing = 0;
		uint8_t  dts_present = 0;
		uint16_t packet_length = 0;
		int64_t  scr = 0, scr_temp = 0;
		int64_t  pts = 0, pts_temp = 0;
		int64_t  dts = 0, dts_temp = 0;
		uint8_t  scr_updated = 0;
		uint8_t  pts_updated = 0;
		uint8_t  dts_updated = 0;
		uint8_t  program_end = 0;
	};

	void get_state(demux_state &state) const;
	void set_state(const demux_state &state);

private:
	enum state : uint8_t
	{
		IDLE, MAGIC0, MAGIC2, MAGIC_MATCH,
		PACK0, PACK1, PACK2, PACK3, PACK4, PACK5,
		PES0, PES1, PES2, PES3, PES4, PES5, PES6, PES7, PES8,
		PES_DTS0, PES_DTS1, PES_DTS2, PES_DTS3, PES_DTS4
	};

	uint8_t  m_stream_class;
	uint8_t  m_state = IDLE;
	bool     m_packet_body = false;
	bool     m_length_decreasing = false;
	uint16_t m_packet_length = 0;
	bool     m_dts_present = false;

	int64_t m_scr = 0, m_scr_temp = 0;
	int64_t m_pts = 0, m_pts_temp = 0;
	int64_t m_dts = 0, m_dts_temp = 0;

	bool m_scr_updated = false;
	bool m_pts_updated = false;
	bool m_dts_updated = false;
	bool m_program_end = false;
};

#endif // MAME_MACHINE_MPEG1DEMUX_H
