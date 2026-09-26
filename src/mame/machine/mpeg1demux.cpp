// license:GPL-3.0-only
// copyright-holders:Andre Zeps, vibecodekun
/******************************************************************************

    ISO/IEC 11172-1 (MPEG-1) program stream demultiplexer

    See mpeg1demux.h. This is a direct port of mpeg_demuxer.sv from the
    CDi_MiSTer VMPEG core, including its habit of parking the state machine in
    MAGIC_MATCH when a stream number does not match.

*******************************************************************************/

#include "mpeg1demux.h"

void mpeg1_demuxer::reset()
{
	m_state = IDLE;
	m_packet_body = false;
	m_length_decreasing = false;
	m_packet_length = 0;
	m_dts_present = false;
	m_scr = m_scr_temp = 0;
	m_pts = m_pts_temp = 0;
	m_dts = m_dts_temp = 0;
	m_scr_updated = m_pts_updated = m_dts_updated = false;
	m_program_end = false;
}

void mpeg1_demuxer::get_state(demux_state &state) const
{
	state.state             = m_state;
	state.packet_body       = m_packet_body ? 1 : 0;
	state.length_decreasing = m_length_decreasing ? 1 : 0;
	state.dts_present       = m_dts_present ? 1 : 0;
	state.packet_length     = m_packet_length;
	state.scr               = m_scr;
	state.scr_temp          = m_scr_temp;
	state.pts               = m_pts;
	state.pts_temp          = m_pts_temp;
	state.dts               = m_dts;
	state.dts_temp          = m_dts_temp;
	state.scr_updated       = m_scr_updated ? 1 : 0;
	state.pts_updated       = m_pts_updated ? 1 : 0;
	state.dts_updated       = m_dts_updated ? 1 : 0;
	state.program_end       = m_program_end ? 1 : 0;
}

void mpeg1_demuxer::set_state(const demux_state &state)
{
	m_state             = state.state;
	m_packet_body       = state.packet_body != 0;
	m_length_decreasing = state.length_decreasing != 0;
	m_dts_present       = state.dts_present != 0;
	m_packet_length     = state.packet_length;
	m_scr               = state.scr;
	m_scr_temp          = state.scr_temp;
	m_pts               = state.pts;
	m_pts_temp          = state.pts_temp;
	m_dts               = state.dts;
	m_dts_temp          = state.dts_temp;
	m_scr_updated       = state.scr_updated != 0;
	m_pts_updated       = state.pts_updated != 0;
	m_dts_updated       = state.dts_updated != 0;
	m_program_end       = state.program_end != 0;
}

// Helper for the 33-bit timestamp fields, which arrive as marker-separated
// 3/15/15 bit groups spread over five bytes.
static inline void put_bits(int64_t &value, int hi, int lo, uint8_t bits)
{
	const int64_t mask = ((int64_t(1) << (hi - lo + 1)) - 1) << lo;
	value = (value & ~mask) | ((int64_t(bits) << lo) & mask);
}

bool mpeg1_demuxer::feed(uint8_t data, uint8_t stream_filter)
{
	// The decoders are gated by the packet-body flag as it stands *before*
	// this byte is processed, matching the registered output in the RTL.
	const bool body = m_packet_body;

	m_scr_updated = m_pts_updated = m_dts_updated = m_program_end = false;

	if (m_length_decreasing)
	{
		if (m_packet_length == 1)
		{
			m_length_decreasing = false;
			m_packet_body = false;
		}
		m_packet_length--;
	}

	switch (m_state)
	{
	case PACK5:
		m_state = IDLE;
		m_scr = m_scr_temp;
		m_scr_updated = true;
		break;

	case PACK4: m_state = PACK5; put_bits(m_scr_temp,  6,  0, (data >> 1) & 0x7f); break;
	case PACK3: m_state = PACK4; put_bits(m_scr_temp, 14,  7, data);               break;
	case PACK2: m_state = PACK3; put_bits(m_scr_temp, 21, 15, (data >> 1) & 0x7f); break;
	case PACK1: m_state = PACK2; put_bits(m_scr_temp, 29, 22, data);               break;
	case PACK0: m_state = PACK1; put_bits(m_scr_temp, 32, 30, (data >> 1) & 0x07); break;

	case PES8:
		// Only reached when a PTS was present; a DTS is optional but can
		// never occur without one.
		m_state = IDLE;
		m_pts = m_pts_temp;
		m_pts_updated = true;
		// Without a DTS the VMPEG uses the PTS in its place.
		m_dts = m_dts_present ? m_dts_temp : m_pts_temp;
		m_dts_updated = true;
		break;

	case PES_DTS4:
		if ((data & 1))
		{
			put_bits(m_dts_temp, 6, 0, (data >> 1) & 0x7f);
			m_packet_body = true;
			m_state = PES8;
		}
		else
		{
			m_state = IDLE;
		}
		break;

	case PES_DTS3: m_state = PES_DTS4; put_bits(m_dts_temp, 14, 7, data); break;

	case PES_DTS2:
		if ((data & 1))
		{
			put_bits(m_dts_temp, 21, 15, (data >> 1) & 0x7f);
			m_state = PES_DTS3;
		}
		else
		{
			m_state = IDLE;
		}
		break;

	case PES_DTS1: m_state = PES_DTS2; put_bits(m_dts_temp, 29, 22, data); break;

	case PES_DTS0:
		if ((data & 0xf1) == 0x11)
		{
			put_bits(m_dts_temp, 32, 30, (data >> 1) & 0x07);
			m_state = PES_DTS1;
		}
		else
		{
			m_state = IDLE;
		}
		break;

	case PES7:
		if ((data & 1))
		{
			put_bits(m_pts_temp, 6, 0, (data >> 1) & 0x7f);
			if (m_dts_present)
			{
				m_state = PES_DTS0;
			}
			else
			{
				m_packet_body = true;
				m_state = PES8;
			}
		}
		else
		{
			m_state = IDLE;
		}
		break;

	case PES6: m_state = PES7; put_bits(m_pts_temp, 14, 7, data); break;

	case PES5:
		if ((data & 1))
		{
			put_bits(m_pts_temp, 21, 15, (data >> 1) & 0x7f);
			m_state = PES6;
		}
		else
		{
			m_state = IDLE;
		}
		break;

	case PES4: m_state = PES5; put_bits(m_pts_temp, 29, 22, data); break;

	case PES2:
		if ((data & 0xf1) == 0x21)          // PTS only
		{
			put_bits(m_pts_temp, 32, 30, (data >> 1) & 0x07);
			m_dts_present = false;
			m_state = PES4;
		}
		else if ((data & 0xf1) == 0x31)     // PTS and DTS
		{
			put_bits(m_pts_temp, 32, 30, (data >> 1) & 0x07);
			m_dts_present = true;
			m_state = PES4;
		}
		else if (data == 0x0f)              // neither
		{
			m_state = IDLE;
			m_packet_body = true;
		}
		else if ((data & 0xc0) == 0x40)     // STD buffer size, two bytes
		{
			m_state = PES3;
		}
		else if (data == 0xff)              // stuffing
		{
			m_state = PES2;
		}
		else
		{
			m_state = IDLE;
		}
		break;

	case PES3:
		// ignore the second STD buffer size byte
		m_state = PES2;
		break;

	case PES1:
		m_state = PES2;
		m_packet_length = (m_packet_length & 0xff00) | data;
		m_length_decreasing = true;
		break;

	case PES0:
		m_state = PES1;
		m_packet_length = (m_packet_length & 0x00ff) | (uint16_t(data) << 8);
		break;

	case MAGIC_MATCH:
		if (data == 0xba)
		{
			m_state = PACK0;
		}
		else if (data == 0xb9)
		{
			m_program_end = true;
			m_state = IDLE;
		}
		else if ((data & 0xf0) == m_stream_class)
		{
			// An elementary stream of the class this demuxer is filtering:
			// 0xCn for audio, 0xEn for video. The MiSTer RTL accepts both
			// classes in both instances, which with the usual stream number
			// of zero lets video packets fall into the audio path; each
			// instance only looks at its own class here.
			//
			// On a stream number mismatch the state machine stays parked in
			// MAGIC_MATCH rather than returning to IDLE, as in the RTL.
			if ((data & 0x0f) == stream_filter)
				m_state = PES0;
		}
		else
		{
			m_state = IDLE;
		}
		break;

	case MAGIC2:
		if (data == 0x01)
			m_state = MAGIC_MATCH;
		else if (data == 0x00)
			m_state = MAGIC2;
		else
			m_state = IDLE;
		break;

	case MAGIC0:
		m_state = (data == 0x00) ? MAGIC2 : IDLE;
		break;

	case IDLE:
	default:
		// A start code prefix is only looked for outside of a packet body.
		if (data == 0x00 && !m_packet_body)
			m_state = MAGIC0;
		else
			m_state = IDLE;
		break;
	}

	return body;
}

