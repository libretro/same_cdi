// license:BSD-3-Clause
// copyright-holders:vibecodekun
/******************************************************************************

    ISO/IEC 11172-3 (MPEG-1) Layer I/II audio decoder

    See mpeg1audio.h for the interface.

    Portions of this file are derived from PL_MPEG by Dominic Szablewski
    (https://github.com/phoboslab/pl_mpeg), whose MP2 decoder is in turn
    based on kjmp2 by Martin J. Fiedler, used under the following license:

    The MIT License (MIT)

    Copyright (c) 2019 Dominic Szablewski

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to permit
    persons to whom the Software is furnished to do so, subject to the
    following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
    NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
    OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
    USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#include "mpeg1audio.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {

constexpr int LAYER_III = 1;
constexpr int LAYER_II  = 2;
constexpr int LAYER_I   = 3;

constexpr int MODE_STEREO       = 0;
constexpr int MODE_JOINT_STEREO = 1;
constexpr int MODE_DUAL_CHANNEL = 2;
constexpr int MODE_MONO         = 3;

const uint16_t SAMPLE_RATE[8] =
{
	44100, 48000, 32000, 0,     // MPEG-1
	22050, 24000, 16000, 0      // MPEG-2
};

const uint16_t BIT_RATE_LAYER_II[28] =
{
	32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,   // MPEG-1
	 8, 16, 24, 32, 40, 48,  56,  64,  80,  96, 112, 128, 144, 160    // MPEG-2
};

const uint16_t BIT_RATE_LAYER_I[28] =
{
	32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, // MPEG-1
	32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256  // MPEG-2
};

// Scale factors are 2 * 2^(-index/3); the three bases cover the residue of the
// index modulo three, and the shift covers the rest.
const int32_t SCALEFACTOR_BASE[3] = { 0x02000000, 0x01965fea, 0x01428a30 };

//**************************************************************************
//  Layer II bit allocation lookup (ISO tables 3-B.2a through 3-B.2d)
//**************************************************************************

// Table selection is by bit rate per channel and then by sample rate. The
// encoded value carries the subband limit in its low six bits and the table
// row set above that.
constexpr uint8_t QUANT_TAB_A = 27 | 64;   // 3-B.2a, high rate, sblimit 27
constexpr uint8_t QUANT_TAB_B = 30 | 64;   // 3-B.2b, high rate, sblimit 30
constexpr uint8_t QUANT_TAB_C = 8;         // 3-B.2c,  low rate, sblimit  8
constexpr uint8_t QUANT_TAB_D = 12;        // 3-B.2d,  low rate, sblimit 12

const uint8_t QUANT_LUT_STEP_1[2][16] =
{
	// 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 kbit/s
	{ 0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2 },   // mono
	// the same, per channel, for two-channel modes
	{ 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2 }    // stereo
};

const uint8_t QUANT_LUT_STEP_2[3][3] =
{
	//  44.1 kHz      48 kHz        32 kHz
	{ QUANT_TAB_C, QUANT_TAB_C, QUANT_TAB_D },   //  32 - 48 kbit/s/ch
	{ QUANT_TAB_A, QUANT_TAB_A, QUANT_TAB_A },   //  56 - 80 kbit/s/ch
	{ QUANT_TAB_B, QUANT_TAB_A, QUANT_TAB_B }    //  96 +   kbit/s/ch
};

// Per subband: number of allocation bits in the high nibble, the row of the
// step 4 table in the low nibble.
const uint8_t QUANT_LUT_STEP_3[3][32] =
{
	// low rate (3-B.2c and 3-B.2d)
	{
		0x44, 0x44,
		0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34
	},
	// high rate (3-B.2a and 3-B.2b)
	{
		0x43, 0x43, 0x43,
		0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
		0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
	},
	// MPEG-2 low sample rate (ISO 13818-3 B.2)
	{
		0x45, 0x45, 0x45, 0x45,
		0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
		0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24,
		0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24
	}
};

// Row of the step 3 table plus the allocation code gives a quantiser index.
const uint8_t QUANT_LUT_STEP_4[6][16] =
{
	{ 0, 1, 2, 17 },
	{ 0, 1, 2,  3, 4, 5, 6, 17 },
	{ 0, 1, 2,  3, 4, 5, 6,  7,  8,  9, 10, 11, 12, 13, 14, 17 },
	{ 0, 1, 3,  5, 6, 7, 8,  9, 10, 11, 12, 13, 14, 15, 16, 17 },
	{ 0, 1, 2,  4, 5, 6, 7,  8,  9, 10, 11, 12, 13, 14, 15, 17 },
	{ 0, 1, 2,  3, 4, 5, 6,  7,  8,  9, 10, 11, 12, 13, 14, 15 }
};

} // anonymous namespace

// Quantiser specifications, indexed by the value the tables above produce.
// Index zero means the subband carries no data.
const mpeg1_detail::quantizer_spec QUANT_TAB[17] =
{
	{     3, 1,  5 }, {     5, 1,  7 }, {     7, 0,  3 }, {     9, 1, 10 },
	{    15, 0,  4 }, {    31, 0,  5 }, {    63, 0,  6 }, {   127, 0,  7 },
	{   255, 0,  8 }, {   511, 0,  9 }, {  1023, 0, 10 }, {  2047, 0, 11 },
	{  4095, 0, 12 }, {  8191, 0, 13 }, { 16383, 0, 14 }, { 32767, 0, 15 },
	{ 65535, 0, 16 }
};

const mpeg1_detail::quantizer_spec QUANT_TAB_LAYER_I[14] =
{
	{     3, 0,  2 }, {     7, 0,  3 }, {    15, 0,  4 }, {    31, 0,  5 },
	{    63, 0,  6 }, {   127, 0,  7 }, {   255, 0,  8 }, {   511, 0,  9 },
	{  1023, 0, 10 }, {  2047, 0, 11 }, {  4095, 0, 12 }, {  8191, 0, 13 },
	{ 16383, 0, 14 }, { 32767, 0, 15 }
};

namespace {

// ISO/IEC 11172-3 Table B.3, the polyphase synthesis window D[i],
// scaled by 65536 so that every entry is an exact integer.
const int32_t SYNTHESIS_WINDOW[512] =
{
	      0,      -1,      -1,      -1,      -1,      -1,      -1,      -2,
	     -2,      -2,      -2,      -3,      -3,      -4,      -4,      -5,
	     -5,      -6,      -7,      -7,      -8,      -9,     -10,     -11,
	    -13,     -14,     -16,     -17,     -19,     -21,     -24,     -26,
	    -29,     -31,     -35,     -38,     -41,     -45,     -49,     -53,
	    -58,     -63,     -68,     -73,     -79,     -85,     -91,     -97,
	   -104,    -111,    -117,    -125,    -132,    -139,    -147,    -154,
	   -161,    -169,    -176,    -183,    -190,    -196,    -202,    -208,
	    213,     218,     222,     225,     227,     228,     228,     227,
	    224,     221,     215,     208,     200,     189,     177,     163,
	    146,     127,     106,      83,      57,      29,      -2,     -36,
	    -72,    -111,    -153,    -197,    -244,    -294,    -347,    -401,
	   -459,    -519,    -581,    -645,    -711,    -779,    -848,    -919,
	   -991,   -1064,   -1137,   -1210,   -1283,   -1356,   -1428,   -1498,
	  -1567,   -1634,   -1698,   -1759,   -1817,   -1870,   -1919,   -1962,
	  -2001,   -2032,   -2057,   -2075,   -2085,   -2087,   -2080,   -2063,
	   2037,    2000,    1952,    1893,    1822,    1739,    1644,    1535,
	   1414,    1280,    1131,     970,     794,     605,     402,     185,
	    -45,    -288,    -545,    -814,   -1095,   -1388,   -1692,   -2006,
	  -2330,   -2663,   -3004,   -3351,   -3705,   -4063,   -4425,   -4788,
	  -5153,   -5517,   -5879,   -6237,   -6589,   -6935,   -7271,   -7597,
	  -7910,   -8209,   -8491,   -8755,   -8998,   -9219,   -9416,   -9585,
	  -9727,   -9838,   -9916,   -9959,   -9966,   -9935,   -9863,   -9750,
	  -9592,   -9389,   -9139,   -8840,   -8492,   -8092,   -7640,   -7134,
	   6574,    5959,    5288,    4561,    3776,    2935,    2037,    1082,
	     70,    -998,   -2122,   -3300,   -4533,   -5818,   -7154,   -8540,
	  -9975,  -11455,  -12980,  -14548,  -16155,  -17799,  -19478,  -21189,
	 -22929,  -24694,  -26482,  -28289,  -30112,  -31947,  -33791,  -35640,
	 -37489,  -39336,  -41176,  -43006,  -44821,  -46617,  -48390,  -50137,
	 -51853,  -53534,  -55178,  -56778,  -58333,  -59838,  -61289,  -62684,
	 -64019,  -65290,  -66494,  -67629,  -68692,  -69679,  -70590,  -71420,
	 -72169,  -72835,  -73415,  -73908,  -74313,  -74630,  -74856,  -74992,
	  75038,   74992,   74856,   74630,   74313,   73908,   73415,   72835,
	  72169,   71420,   70590,   69679,   68692,   67629,   66494,   65290,
	  64019,   62684,   61289,   59838,   58333,   56778,   55178,   53534,
	  51853,   50137,   48390,   46617,   44821,   43006,   41176,   39336,
	  37489,   35640,   33791,   31947,   30112,   28289,   26482,   24694,
	  22929,   21189,   19478,   17799,   16155,   14548,   12980,   11455,
	   9975,    8540,    7154,    5818,    4533,    3300,    2122,     998,
	    -70,   -1082,   -2037,   -2935,   -3776,   -4561,   -5288,   -5959,
	   6574,    7134,    7640,    8092,    8492,    8840,    9139,    9389,
	   9592,    9750,    9863,    9935,    9966,    9959,    9916,    9838,
	   9727,    9585,    9416,    9219,    8998,    8755,    8491,    8209,
	   7910,    7597,    7271,    6935,    6589,    6237,    5879,    5517,
	   5153,    4788,    4425,    4063,    3705,    3351,    3004,    2663,
	   2330,    2006,    1692,    1388,    1095,     814,     545,     288,
	     45,    -185,    -402,    -605,    -794,    -970,   -1131,   -1280,
	  -1414,   -1535,   -1644,   -1739,   -1822,   -1893,   -1952,   -2000,
	   2037,    2063,    2080,    2087,    2085,    2075,    2057,    2032,
	   2001,    1962,    1919,    1870,    1817,    1759,    1698,    1634,
	   1567,    1498,    1428,    1356,    1283,    1210,    1137,    1064,
	    991,     919,     848,     779,     711,     645,     581,     519,
	    459,     401,     347,     294,     244,     197,     153,     111,
	     72,      36,       2,     -29,     -57,     -83,    -106,    -127,
	   -146,    -163,    -177,    -189,    -200,    -208,    -215,    -221,
	   -224,    -227,    -228,    -228,    -227,    -225,    -222,    -218,
	    213,     208,     202,     196,     190,     183,     176,     169,
	    161,     154,     147,     139,     132,     125,     117,     111,
	    104,      97,      91,      85,      79,      73,      68,      63,
	     58,      53,      49,      45,      41,      38,      35,      31,
	     29,      26,      24,      21,      19,      17,      16,      14,
	     13,      11,      10,       9,       8,       7,       7,       6,
	      5,       5,       4,       4,       3,       3,       2,       2,
	      2,       2,       1,       1,       1,       1,       1,       1,
};

// Matrixing coefficients N[i][k] = cos((16 + i) * (2k + 1) * pi / 64),
// built once on first use.
struct matrix_table
{
	double n[64][32];

	matrix_table()
	{
		for (int i = 0; i < 64; i++)
			for (int k = 0; k < 32; k++)
				n[i][k] = std::cos((16 + i) * (2 * k + 1) * 3.14159265358979323846 / 64.0);
	}
};

const matrix_table &matrix()
{
	static const matrix_table table;
	return table;
}

inline int16_t clamp_sample(double value)
{
	const int v = int(value);
	return int16_t((v < -32768) ? -32768 : ((v > 32767) ? 32767 : v));
}

} // anonymous namespace

//**************************************************************************
//  Construction
//**************************************************************************

mpeg1_audio_decoder::mpeg1_audio_decoder()
{
	reset();
}

void mpeg1_audio_decoder::reset()
{
	m_buffer.clear();
	m_read_pos = 0;
	m_bit_pos = 0;
	m_underflow = false;
	m_frame_decoded = false;

	m_frame_header = 0;
	m_version = 0;
	m_layer = 0;
	m_bitrate_index = 0;
	m_samplerate_index = 3;
	m_mode = 0;
	m_bound = 0;
	m_sample_rate = 0;
	m_bit_rate = 0;
	m_channels = 2;
	m_samples_per_frame = 1152;
	m_next_frame_size = 0;
	m_has_header = false;

	std::memset(m_allocation, 0, sizeof(m_allocation));
	std::memset(m_scale_factor_info, 0, sizeof(m_scale_factor_info));
	std::memset(m_scale_factor, 0, sizeof(m_scale_factor));
	std::memset(m_sample, 0, sizeof(m_sample));
	std::memset(m_v, 0, sizeof(m_v));
	m_v_pos = 0;

	m_output.assign(1152 * 2, 0);
	m_sample_count = 0;
}

void mpeg1_audio_decoder::get_decoder_state(decoder_state &state) const
{
	// Rebase so the read position becomes byte 0; nothing behind it is read
	// again. Only a driver that ignored the FMA status entirely could push the
	// buffer past the cap, and the clamp keeps that from overrunning the copy.
	const size_t available = m_buffer.size() - m_read_pos;
	const size_t size = std::min<size_t>(available, decoder_state::max_buffer);
	state.buffer_size = uint32_t(size);
	state.bit_pos = uint32_t(std::min<size_t>(m_bit_pos - m_read_pos * 8, size * 8));
	if (size != 0)
		std::memcpy(state.buffer, m_buffer.data() + m_read_pos, size);

	state.underflow = m_underflow ? 1 : 0;
	state.frame_decoded = m_frame_decoded ? 1 : 0;
	state.has_header = m_has_header ? 1 : 0;

	state.frame_header = m_frame_header;
	state.version = m_version;
	state.layer = m_layer;
	state.bitrate_index = m_bitrate_index;
	state.samplerate_index = m_samplerate_index;
	state.mode = m_mode;
	state.bound = m_bound;
	state.sample_rate = m_sample_rate;
	state.bit_rate = m_bit_rate;
	state.channels = m_channels;
	state.samples_per_frame = m_samples_per_frame;
	state.next_frame_size = m_next_frame_size;

	std::memcpy(state.v, m_v, sizeof(state.v));
	state.v_pos = m_v_pos;
}

void mpeg1_audio_decoder::set_decoder_state(const decoder_state &state)
{
	const size_t size = std::min<size_t>(state.buffer_size, decoder_state::max_buffer);
	m_buffer.assign(state.buffer, state.buffer + size);
	m_read_pos = 0;
	m_bit_pos = std::min<size_t>(state.bit_pos, size * 8);

	m_underflow = state.underflow != 0;
	m_frame_decoded = state.frame_decoded != 0;
	m_has_header = state.has_header != 0;

	m_frame_header = state.frame_header;
	m_version = state.version;
	m_layer = state.layer;
	m_bitrate_index = state.bitrate_index;
	m_samplerate_index = state.samplerate_index;
	m_mode = state.mode;
	m_bound = state.bound;
	m_sample_rate = state.sample_rate;
	m_bit_rate = state.bit_rate;
	m_channels = state.channels;
	m_samples_per_frame = state.samples_per_frame;
	m_next_frame_size = state.next_frame_size;

	std::memcpy(m_v, state.v, sizeof(m_v));
	m_v_pos = state.v_pos;

	// Per-frame working state is rebuilt by the next decode(); clear the
	// pointer table so a stale entry can never be followed if that assumption
	// is ever broken.
	std::memset(m_allocation, 0, sizeof(m_allocation));
	m_sample_count = 0;
}

//**************************************************************************
//  Bit reading
//**************************************************************************

void mpeg1_audio_decoder::write(const uint8_t *data, size_t length)
{
	if (m_buffer.size() - m_read_pos + length > m_buffer_limit)
	{
		const size_t drop = std::min(m_read_pos, m_buffer.size());
		if (drop)
		{
			m_buffer.erase(m_buffer.begin(), m_buffer.begin() + drop);
			m_bit_pos -= drop * 8;
			m_read_pos = 0;
		}
	}

	m_buffer.insert(m_buffer.end(), data, data + length);
}

bool mpeg1_audio_decoder::have_bits(size_t count) const
{
	return m_bit_pos + count <= m_buffer.size() * 8;
}

uint32_t mpeg1_audio_decoder::read_bits(int count)
{
	uint32_t value = 0;
	while (count--)
	{
		if (m_bit_pos >= m_buffer.size() * 8)
		{
			value <<= 1;
			m_bit_pos++;
			continue;
		}
		value = (value << 1) | ((m_buffer[m_bit_pos >> 3] >> (7 - (m_bit_pos & 7))) & 1);
		m_bit_pos++;
	}
	return value;
}

void mpeg1_audio_decoder::align_to_byte()
{
	m_bit_pos = (m_bit_pos + 7) & ~size_t(7);
}

//**************************************************************************
//  Frame header
//**************************************************************************

bool mpeg1_audio_decoder::find_frame_sync()
{
	// A frame begins with eleven set bits; scan byte-aligned since that is
	// where a valid frame always starts.
	for (size_t i = m_bit_pos >> 3; i + 1 < m_buffer.size(); i++)
	{
		if (m_buffer[i] == 0xff && (m_buffer[i + 1] & 0xe0) == 0xe0)
		{
			m_bit_pos = i * 8;
			return true;
		}
	}

	// Nothing found; drop what has been scanned so the buffer does not grow.
	m_bit_pos = (m_buffer.size() > 1) ? (m_buffer.size() - 1) * 8 : 0;
	return false;
}

bool mpeg1_audio_decoder::decode_header()
{
	if (!have_bits(32))
		return false;

	const size_t start = m_bit_pos;
	const uint32_t header = read_bits(32);

	if ((header & 0xffe00000) != 0xffe00000)
	{
		m_bit_pos = start + 8;
		return false;
	}

	const int version = (header >> 19) & 3;
	const int layer = (header >> 17) & 3;
	const int bitrate_index = (header >> 12) & 0x0f;
	const int samplerate_index = (header >> 10) & 3;
	const int padding = (header >> 9) & 1;
	const int mode = (header >> 6) & 3;

	// Only MPEG-1 (version 3) and MPEG-2 (version 2) Layer I/II are handled.
	if (version == 1 || layer == 0 || layer == LAYER_III ||
		bitrate_index == 0 || bitrate_index == 0x0f || samplerate_index == 3)
	{
		m_bit_pos = start + 8;
		return false;
	}

	m_frame_header = header;
	m_version = version;
	m_layer = layer;
	m_bitrate_index = bitrate_index - 1;
	m_samplerate_index = samplerate_index;
	m_mode = mode;

	const bool mpeg2 = (version == 2);
	m_sample_rate = SAMPLE_RATE[samplerate_index + (mpeg2 ? 4 : 0)];
	m_bit_rate = (layer == LAYER_I)
		? BIT_RATE_LAYER_I[m_bitrate_index + (mpeg2 ? 14 : 0)]
		: BIT_RATE_LAYER_II[m_bitrate_index + (mpeg2 ? 14 : 0)];

	m_channels = (mode == MODE_MONO) ? 1 : 2;

	// In joint stereo the mode extension gives the number of subbands that are
	// still coded separately.
	m_bound = (mode == MODE_JOINT_STEREO) ? ((((header >> 4) & 3) + 1) << 2) : 32;
	if (mode == MODE_MONO)
		m_bound = 0;

	if (layer == LAYER_I)
	{
		m_samples_per_frame = 384;
		m_next_frame_size = (12000 * m_bit_rate / m_sample_rate + padding) * 4;
	}
	else
	{
		m_samples_per_frame = 1152;
		m_next_frame_size = 144000 * m_bit_rate / m_sample_rate + padding;
	}

	m_has_header = true;
	return true;
}

//**************************************************************************
//  Frame decoding
//**************************************************************************

int mpeg1_audio_decoder::decode()
{
	m_sample_count = 0;

	for (;;)
	{
		const size_t frame_start = m_bit_pos;

		if (!find_frame_sync())
		{
			m_underflow = true;
			return 0;
		}

		if (!decode_header())
		{
			if (m_bit_pos <= frame_start)
				return 0;
			continue;
		}

		// The whole frame has to be present before it can be decoded.
		const size_t frame_bits = size_t(m_next_frame_size) * 8;
		const size_t header_start = m_bit_pos - 32;
		if (header_start + frame_bits > m_buffer.size() * 8)
		{
			m_bit_pos = header_start;
			m_underflow = true;
			return 0;
		}

		m_underflow = false;

		// Skip the CRC when protection is enabled.
		if (((m_frame_header >> 16) & 1) == 0)
			read_bits(16);

		if (m_layer == LAYER_I)
			decode_layer_i();
		else
			decode_layer_ii();

		// Move to the start of the next frame regardless of how many bits the
		// payload actually used.
		m_bit_pos = header_start + frame_bits;
		m_read_pos = m_bit_pos >> 3;

		if (m_read_pos >= 16384)
		{
			m_buffer.erase(m_buffer.begin(), m_buffer.begin() + m_read_pos);
			m_bit_pos -= m_read_pos * 8;
			m_read_pos = 0;
		}

		m_frame_decoded = true;
		return m_sample_count;
	}
}

const mpeg1_audio_decoder::quantizer_spec *mpeg1_audio_decoder::read_allocation(int sb, int table)
{
	const int step3 = QUANT_LUT_STEP_3[table][sb];
	const int bits = step3 >> 4;
	const int row = step3 & 15;

	const int code = int(read_bits(bits));
	const int index = QUANT_LUT_STEP_4[row][code];

	return index ? &QUANT_TAB[index - 1] : nullptr;
}

void mpeg1_audio_decoder::read_samples(int channel, int sb, int part)
{
	const quantizer_spec *const q = m_allocation[channel][sb];
	double *const out = m_sample[channel][sb];

	if (!q)
	{
		out[0] = out[1] = out[2] = 0.0;
		return;
	}

	// Resolve the scale factor: 2 * 2^(-index/3), with 63 meaning silence.
	const int sf_index = m_scale_factor[channel][sb][part];
	double scale;
	if (sf_index == 63)
	{
		scale = 0.0;
	}
	else
	{
		const int shift = sf_index / 3;
		scale = double(SCALEFACTOR_BASE[sf_index % 3]) / double(1 << 24) / double(1 << shift);
	}

	int code[3];
	if (q->group)
	{
		// Three samples share one codeword.
		uint32_t value = read_bits(q->bits);
		code[0] = int(value % q->levels);
		value /= q->levels;
		code[1] = int(value % q->levels);
		code[2] = int(value / q->levels);
	}
	else
	{
		code[0] = int(read_bits(q->bits));
		code[1] = int(read_bits(q->bits));
		code[2] = int(read_bits(q->bits));
	}

	// The code is a two's complement fraction with its most significant bit
	// inverted, so subtracting from the midpoint recovers the signed value.
	const int adjust = ((q->levels + 1) >> 1) - 1;
	const double norm = 1.0 / double(q->levels + 1);

	for (int i = 0; i < 3; i++)
		out[i] = double(adjust - code[i]) * norm * scale;
}

void mpeg1_audio_decoder::decode_layer_ii()
{
	const int table1 = (m_mode == MODE_MONO) ? 0 : 1;
	const int table2 = QUANT_LUT_STEP_1[table1][m_bitrate_index];
	const int encoded = QUANT_LUT_STEP_2[table2][m_samplerate_index];

	int sblimit = encoded & 63;
	const int table = encoded >> 6;

	int bound = (m_mode == MODE_JOINT_STEREO) ? m_bound : ((m_mode == MODE_MONO) ? 0 : 32);
	if (bound > sblimit)
		bound = sblimit;

	// Bit allocation
	std::memset(m_allocation, 0, sizeof(m_allocation));
	for (int sb = 0; sb < bound; sb++)
	{
		m_allocation[0][sb] = read_allocation(sb, table);
		m_allocation[1][sb] = read_allocation(sb, table);
	}
	for (int sb = bound; sb < sblimit; sb++)
		m_allocation[0][sb] = m_allocation[1][sb] = read_allocation(sb, table);

	// Scale factor selection information
	for (int sb = 0; sb < sblimit; sb++)
	{
		for (int ch = 0; ch < m_channels; ch++)
		{
			if (m_allocation[ch][sb])
				m_scale_factor_info[ch][sb] = uint8_t(read_bits(2));
		}
		if (m_mode == MODE_MONO)
			m_scale_factor_info[1][sb] = m_scale_factor_info[0][sb];
	}

	// Scale factors
	for (int sb = 0; sb < sblimit; sb++)
	{
		for (int ch = 0; ch < m_channels; ch++)
		{
			if (!m_allocation[ch][sb])
				continue;

			int *const sf = m_scale_factor[ch][sb];
			switch (m_scale_factor_info[ch][sb])
			{
			case 0:
				sf[0] = int(read_bits(6));
				sf[1] = int(read_bits(6));
				sf[2] = int(read_bits(6));
				break;
			case 1:
				sf[0] = sf[1] = int(read_bits(6));
				sf[2] = int(read_bits(6));
				break;
			case 2:
				sf[0] = sf[1] = sf[2] = int(read_bits(6));
				break;
			default:
				sf[0] = int(read_bits(6));
				sf[1] = sf[2] = int(read_bits(6));
				break;
			}
		}

		if (m_mode == MODE_MONO)
		{
			m_scale_factor[1][sb][0] = m_scale_factor[0][sb][0];
			m_scale_factor[1][sb][1] = m_scale_factor[0][sb][1];
			m_scale_factor[1][sb][2] = m_scale_factor[0][sb][2];
		}
	}

	// Twelve granules, each producing three sets of 32 subband samples.
	int out_pos = 0;
	for (int granule = 0; granule < 12; granule++)
	{
		const int part = granule >> 2;

		for (int sb = 0; sb < bound; sb++)
		{
			read_samples(0, sb, part);
			read_samples(1, sb, part);
		}

		for (int sb = bound; sb < sblimit; sb++)
		{
			read_samples(0, sb, part);
			// Above the bound both channels share the coded samples.
			for (int i = 0; i < 3; i++)
				m_sample[1][sb][i] = m_sample[0][sb][i];
		}

		for (int sb = sblimit; sb < 32; sb++)
		{
			for (int ch = 0; ch < 2; ch++)
				m_sample[ch][sb][0] = m_sample[ch][sb][1] = m_sample[ch][sb][2] = 0.0;
		}

		for (int part_index = 0; part_index < 3; part_index++)
			synthesise(part_index, out_pos);
	}

	m_sample_count = out_pos / 2;
}

void mpeg1_audio_decoder::decode_layer_i()
{
	int bound = (m_mode == MODE_JOINT_STEREO) ? m_bound : ((m_mode == MODE_MONO) ? 0 : 32);

	std::memset(m_allocation, 0, sizeof(m_allocation));

	for (int sb = 0; sb < bound; sb++)
	{
		for (int ch = 0; ch < 2; ch++)
		{
			const int code = int(read_bits(4));
			m_allocation[ch][sb] = code ? &QUANT_TAB_LAYER_I[code - 1] : nullptr;
		}
	}
	for (int sb = bound; sb < 32; sb++)
	{
		const int code = int(read_bits(4));
		m_allocation[0][sb] = m_allocation[1][sb] = code ? &QUANT_TAB_LAYER_I[code - 1] : nullptr;
	}

	for (int sb = 0; sb < 32; sb++)
	{
		for (int ch = 0; ch < m_channels; ch++)
		{
			if (m_allocation[ch][sb])
				m_scale_factor[ch][sb][0] = int(read_bits(6));
		}
		if (m_mode == MODE_MONO)
			m_scale_factor[1][sb][0] = m_scale_factor[0][sb][0];
	}

	int out_pos = 0;
	for (int granule = 0; granule < 12; granule++)
	{
		for (int sb = 0; sb < bound; sb++)
		{
			for (int ch = 0; ch < 2; ch++)
			{
				const quantizer_spec *const q = m_allocation[ch][sb];
				if (!q)
				{
					m_sample[ch][sb][0] = 0.0;
					continue;
				}

				const int sf_index = m_scale_factor[ch][sb][0];
				const int shift = sf_index / 3;
				const double scale = (sf_index == 63) ? 0.0
					: double(SCALEFACTOR_BASE[sf_index % 3]) / double(1 << 24) / double(1 << shift);

				const int code = int(read_bits(q->bits));
				const int adjust = ((q->levels + 1) >> 1) - 1;
				m_sample[ch][sb][0] = double(adjust - code) / double(q->levels + 1) * scale;
			}
		}

		for (int sb = bound; sb < 32; sb++)
		{
			const quantizer_spec *const q = m_allocation[0][sb];
			if (!q)
			{
				m_sample[0][sb][0] = m_sample[1][sb][0] = 0.0;
				continue;
			}

			const int sf_index = m_scale_factor[0][sb][0];
			const int shift = sf_index / 3;
			const double scale = (sf_index == 63) ? 0.0
				: double(SCALEFACTOR_BASE[sf_index % 3]) / double(1 << 24) / double(1 << shift);

			const int code = int(read_bits(q->bits));
			const int adjust = ((q->levels + 1) >> 1) - 1;
			m_sample[0][sb][0] = m_sample[1][sb][0] = double(adjust - code) / double(q->levels + 1) * scale;
		}

		synthesise(0, out_pos);
	}

	m_sample_count = out_pos / 2;
}

//**************************************************************************
//  Polyphase synthesis filter (ISO/IEC 11172-3 clause 2.4.3.2)
//**************************************************************************

void mpeg1_audio_decoder::synthesise(int part, int &out_pos)
{
	m_v_pos = (m_v_pos - 64) & 1023;

	const matrix_table &n = matrix();

	for (int ch = 0; ch < 2; ch++)
	{
		// Matrixing: 32 subband samples become 64 values in the history.
		double *const v = &m_v[ch][0];
		for (int i = 0; i < 64; i++)
		{
			double sum = 0.0;
			for (int k = 0; k < 32; k++)
				sum += n.n[i][k] * m_sample[ch][k][part];
			v[(m_v_pos + i) & 1023] = sum;
		}

		// Build the 512-entry window input and apply the window.
		double u[512];
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 32; j++)
			{
				u[i * 64 + j]      = v[(m_v_pos + i * 128 + j) & 1023];
				u[i * 64 + 32 + j] = v[(m_v_pos + i * 128 + 96 + j) & 1023];
			}
		}

		// The window table is stored scaled by 65536.
		double out[32];
		for (int j = 0; j < 32; j++)
		{
			double sum = 0.0;
			for (int i = 0; i < 16; i++)
				sum += u[j + 32 * i] * (double(SYNTHESIS_WINDOW[j + 32 * i]) / 65536.0);
			out[j] = sum;
		}

		for (int j = 0; j < 32; j++)
			m_output[out_pos + j * 2 + ch] = clamp_sample(out[j] * 32767.0);
	}

	// A mono stream fills only channel zero, so mirror it.
	if (m_mode == MODE_MONO)
	{
		for (int j = 0; j < 32; j++)
			m_output[out_pos + j * 2 + 1] = m_output[out_pos + j * 2];
	}

	out_pos += 64;
}
