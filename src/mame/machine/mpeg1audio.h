// license:BSD-3-Clause
// copyright-holders:vibecodekun
/******************************************************************************

    ISO/IEC 11172-3 (MPEG-1) Layer I/II audio decoder

    Companion to mpeg1video.h for the CD-i Digital Video Cartridge, and like it
    deliberately independent of MAME so it can be tested on its own. The
    quantiser tables follow pl_mpeg and kjmp2; see mpeg1audio.cpp for pl_mpeg's
    MIT license notice.

    Green Book streams are Layer II, 44.1 kHz, 224 kbit/s stereo, but Layer I
    and the other sample rates are handled as well since they cost almost
    nothing to support.

*******************************************************************************/

#ifndef MAME_MACHINE_MPEG1AUDIO_H
#define MAME_MACHINE_MPEG1AUDIO_H

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace mpeg1_detail {

// One entry of the quantiser tables: how many levels the subband is coded
// with, whether three samples share a codeword, and the codeword width.
struct quantizer_spec
{
	uint16_t levels;
	uint8_t group;
	uint8_t bits;
};

} // namespace mpeg1_detail

class mpeg1_audio_decoder
{
public:
	// Layer II carries 1152 samples per channel per frame, which is the most
	// any one call to decode() can produce.
	static constexpr int max_samples_per_frame = 1152;

	mpeg1_audio_decoder();

	void reset();

	// Feed demultiplexed elementary stream bytes.
	void write(const uint8_t *data, size_t length);

	// Decode one frame. Returns the number of sample pairs produced, or zero
	// if more data is needed.
	int decode();

	// Interleaved stereo output of the last decoded frame. Mono streams are
	// duplicated to both channels so the consumer always sees stereo.
	const int16_t *samples() const { return m_output.data(); }
	int sample_count() const { return m_sample_count; }

	int sample_rate() const { return m_sample_rate; }
	int channels() const { return m_channels; }
	int bit_rate() const { return m_bit_rate; }

	// The 32 bits of the most recent frame header, which the cartridge exposes
	// to the driver verbatim.
	uint32_t frame_header() const { return m_frame_header; }

	// Set when a frame has been decoded since the last call to this.
	bool take_frame_decoded() { const bool v = m_frame_decoded; m_frame_decoded = false; return v; }

	size_t buffered_bytes() const { return m_buffer.size() - m_read_pos; }
	void set_buffer_limit(size_t bytes) { m_buffer_limit = bytes; }
	bool underflowed() const { return m_underflow; }
	void clear_underflow() { m_underflow = false; }

	// Everything that has to survive a save state.
	//
	// The per-frame working set - the allocation table, scale factors, the
	// dequantised samples and the output buffer - is deliberately absent.
	// decode() fills and consumes all of it within the one call, and the
	// allocation table is an array of pointers into static tables that would
	// have to be turned back into indices for no benefit.
	//
	// What does have to come back is the polyphase synthesis history. It is a
	// rolling window that spans frames, and starting it from zero puts a click
	// on the first frame after every load.
	struct decoder_state
	{
		// write() compacts but does not refuse data once the limit is reached,
		// so unlike the video decoder the buffer can run past its limit if the
		// driver keeps pushing. Four times the cartridge's 64KB gives that
		// plenty of room; a buffer somehow larger still is truncated, costing
		// the tail of the stream and resyncing at the next frame header.
		static constexpr size_t max_buffer = 256 * 1024;

		uint32_t buffer_size = 0;
		uint32_t bit_pos = 0;               // bit offset into buffer[]
		uint8_t  buffer[max_buffer] = { 0 };

		uint8_t  underflow = 0;
		uint8_t  frame_decoded = 0;
		uint8_t  has_header = 0;

		uint32_t frame_header = 0;
		int32_t  version = 0;
		int32_t  layer = 0;
		int32_t  bitrate_index = 0;
		int32_t  samplerate_index = 3;
		int32_t  mode = 0;
		int32_t  bound = 0;
		int32_t  sample_rate = 0;
		int32_t  bit_rate = 0;
		int32_t  channels = 2;
		int32_t  samples_per_frame = 1152;
		int32_t  next_frame_size = 0;

		double   v[2][1024] = { { 0.0 } };
		int32_t  v_pos = 0;
	};

	void get_decoder_state(decoder_state &state) const;
	void set_decoder_state(const decoder_state &state);

private:
	using quantizer_spec = mpeg1_detail::quantizer_spec;

	bool have_bits(size_t count) const;
	uint32_t read_bits(int count);
	void align_to_byte();

	bool find_frame_sync();
	bool decode_header();
	void decode_layer_ii();
	void decode_layer_i();
	const quantizer_spec *read_allocation(int sb, int table);
	void read_samples(int channel, int sb, int part);
	void synthesise(int part, int &out_pos);

	std::vector<uint8_t> m_buffer;
	size_t m_read_pos = 0;
	size_t m_bit_pos = 0;
	size_t m_buffer_limit = 128 * 1024;
	bool m_underflow = false;
	bool m_frame_decoded = false;

	// header fields
	uint32_t m_frame_header = 0;
	int m_version = 0;
	int m_layer = 0;
	int m_bitrate_index = 0;
	int m_samplerate_index = 3;
	int m_mode = 0;
	int m_bound = 0;
	int m_sample_rate = 0;
	int m_bit_rate = 0;
	int m_channels = 2;
	int m_samples_per_frame = 1152;
	int m_next_frame_size = 0;
	bool m_has_header = false;

	const quantizer_spec *m_allocation[2][32];
	uint8_t m_scale_factor_info[2][32];
	int m_scale_factor[2][32][3];
	double m_sample[2][32][3];

	// polyphase synthesis history
	double m_v[2][1024];
	int m_v_pos = 0;

	std::vector<int16_t> m_output;
	int m_sample_count = 0;
};

#endif // MAME_MACHINE_MPEG1AUDIO_H
