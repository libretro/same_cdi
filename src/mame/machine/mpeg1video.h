// license:BSD-3-Clause
// copyright-holders:vibecodekun
/******************************************************************************

    ISO/IEC 11172-2 (MPEG-1) video decoder

    Written for the CD-i Digital Video Cartridge, but deliberately free of any
    dependency on MAME so that it can be exercised standalone against captured
    streams.

    The decoder is fed a demultiplexed video elementary stream through write()
    and decodes one picture per call to decode(). Pictures are only decoded
    once the whole picture is buffered, so the caller never has to deal with
    partially consumed input.

    The variable-length code tables, the zig-zag order, the default quantiser
    matrices and the coefficient escape rules are all defined by the standard;
    they are transcribed here in the binary-tree form used by pl_mpeg, which
    the CDi_MiSTer project also uses for its decoder firmware. pl_mpeg's MIT
    license notice is reproduced in mpeg1video.cpp.

*******************************************************************************/

#ifndef MAME_MACHINE_MPEG1VIDEO_H
#define MAME_MACHINE_MPEG1VIDEO_H

#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace mpeg1_detail {

// A node of a canonical VLC tree: on each bit, either descend to another node
// or emit a value. An index of 0 means the value is final, -1 that the code
// cannot occur.
struct vlc_node
{
	int16_t index;
	int16_t value;
};

struct vlc_node_u
{
	int16_t index;
	uint16_t value;
};

} // namespace mpeg1_detail

class mpeg1_video_decoder
{
public:
	mpeg1_video_decoder();

	void reset();
	// Discard input and reference pictures while retaining sequence parameters
	// for a seek whose new position may not contain another sequence header.
	void clear_fifo();

	// Feed demultiplexed elementary stream bytes.
	void write(const uint8_t *data, size_t length);

	// Decode until one picture has been produced. Returns true if a picture
	// was decoded, false if more data is needed.
	bool decode();

	bool has_sequence() const { return m_has_sequence; }
	int width() const { return m_width; }
	int height() const { return m_height; }

	// Frame period of the coded sequence, in 90 kHz ticks, and the raw frame
	// rate index from the sequence header.
	uint16_t frame_period_90khz() const;
	uint8_t frame_rate_index() const { return m_frame_rate_index; }

	// Picture type of the most recently decoded picture (1=I, 2=P, 3=B) and
	// its temporal reference.
	int picture_type() const { return m_picture_type; }
	int temporal_reference() const { return m_temporal_reference; }
	// VMPEG packs GOP seconds/pictures into the high word and hours/minutes
	// into the low word. Keep this metadata with each reordered picture.
	uint32_t decoded_timecode() const { return m_gop_timecode; }
	uint32_t display_timecode() const { return m_display_frame ? m_display_frame->timecode : 0; }

	// True once a picture has been decoded that is ready to be shown. Frames
	// come out in display order, so an I or P picture is held back until the
	// following anchor picture arrives.
	bool has_display_frame() const { return m_display_frame != nullptr; }

	// Single-picture/scan mode displays the decoded picture immediately,
	// without waiting for the next anchor to reorder an I or P picture.
	void display_decoded_frame();

	// Picture type and temporal reference of the frame currently being shown,
	// which because of reordering is not the one most recently decoded.
	int display_picture_type() const { return m_display_frame ? m_display_frame->picture_type : 0; }
	int display_temporal_reference() const { return m_display_frame ? m_display_frame->temporal_reference : 0; }

	// Converts the current display frame to packed 0x00RRGGBB. dst must have
	// room for width() * height() entries; stride is in pixels.
	void display_frame_rgb(uint32_t *dst, int stride) const;

	// Raw planar access to the current display frame, for testing.
	const uint8_t *display_y() const;
	const uint8_t *display_cb() const;
	const uint8_t *display_cr() const;
	int luma_stride() const { return m_luma_width; }
	int chroma_stride() const { return m_chroma_width; }

	// Number of whole pictures currently sitting in the input buffer, which
	// is what the cartridge reports to the driver.
	int pictures_buffered() const;

	size_t buffered_bytes() const { return m_buffer.size() - m_read_pos; }

	// Clamped so that the save state's fixed-size copy of the buffer can never
	// be too small for it. See decoder_state.
	void set_buffer_limit(size_t bytes)
	{
		m_buffer_limit = (bytes > decoder_state::max_buffer) ? decoder_state::max_buffer : bytes;
	}

	// True while the buffer holds too much undecoded data to keep accepting
	// more, which is what the cartridge reports to the driver through SYS_STS.
	//
	// This is the live state of the buffer rather than a latch: the hardware
	// compares the FIFO level against a high-water mark continuously, so the
	// condition goes away again on its own once the decoder has drained enough.
	// The mark sits below the limit so that the driver is told to stop while
	// there is still room, and nothing has to be dropped. The reference FIFO
	// holds 32768 bytes and reports full above 28000 of them; the same fraction
	// is used here so that the headroom scales with the buffer.
	bool buffer_full() const { return buffered_bytes() > uint64_t(m_buffer_limit) * 28000 / 32768; }

	// Set when a sequence end code has been seen.
	bool sequence_ended() const { return m_sequence_ended; }
	void clear_sequence_ended() { m_sequence_ended = false; }

	// Everything a sequence header establishes. A stream only carries one
	// every so often, so a decoder that is started part way through a clip
	// (after a save state is restored, say) has to be told these separately or
	// it will discard pictures until the next header comes round, which may be
	// never. Kept as plain data so the owning device can save it.
	struct sequence_state
	{
		uint8_t valid = 0;
		uint16_t width = 0;
		uint16_t height = 0;
		uint8_t frame_rate_index = 0;
		uint8_t intra_quant[64] = { 0 };
		uint8_t non_intra_quant[64] = { 0 };
	};

	void get_sequence_state(sequence_state &state) const;
	void set_sequence_state(const sequence_state &state);

	// Everything else a save state has to carry.
	//
	// The bit reader, picture, slice and macroblock fields are deliberately
	// absent. decode() only ever returns with the bit position parked on a
	// start code - every early exit rewinds to one - so a picture is never half
	// decoded between calls and none of that state is live across a save.
	//
	// What cannot be reconstructed is the pair of reference pictures. Without
	// them the first P and B pictures after a load predict from whatever the
	// buffers happened to hold, which shows as a few frames of smeared blocks,
	// so they ride along in full.
	struct decoder_state
	{
		// The input buffer is capped at the largest the decoder will accept
		// (set_buffer_limit() clamps to this), so the copy can never truncate.
		// The frame cap covers the biggest picture a Green Book stream carries,
		// 384x280 rounded up to whole macroblocks. A larger picture still saves
		// and loads correctly, just without its reference pictures: frames_saved
		// comes back 0 and the stream resyncs on the next intra picture, which
		// is what the hardware does across a seek anyway.
		static constexpr size_t max_buffer = 512 * 1024;
		static constexpr size_t max_luma   = 384 * 288;
		static constexpr size_t max_chroma = 192 * 144;

		// Undecoded input, rebased so that the read position is byte 0.
		uint32_t buffer_size = 0;
		uint32_t bit_pos = 0;               // bit offset into buffer[]
		uint8_t  buffer[max_buffer] = { 0 };

		uint8_t  sequence_ended = 0;
		// Low three bits are picture type; the remaining bits carry the GOP
		// timecode shifted left by three. Its highest bit is bit 27, so this
		// fits a positive int32_t. Legacy states have zero in the unused bits
		// and recover timecodes when the next GOP header arrives. This retains
		// the existing save layout and all input-buffer capacity.
		int32_t  picture_type = 0;
		int32_t  temporal_reference = 0;

		// Frame store. The four working pointers are saved as indices into
		// frames[], with -1 standing in for a null display frame.
		uint8_t  frames_saved = 0;
		int32_t  past_index = 1;
		int32_t  future_index = 2;
		int32_t  work_index = 0;
		int32_t  display_index = -1;
		uint8_t  frame_valid[3] = { 0, 0, 0 };
		int32_t  frame_picture_type[3] = { 0, 0, 0 }; // same packed metadata
		int32_t  frame_temporal_reference[3] = { 0, 0, 0 };
		uint8_t  frame_y[3][max_luma] = { { 0 } };
		uint8_t  frame_cb[3][max_chroma] = { { 0 } };
		uint8_t  frame_cr[3][max_chroma] = { { 0 } };
	};

	// Must be called after set_sequence_state(), which reallocates and clears
	// the frame store whenever the picture size changes.
	void get_decoder_state(decoder_state &state) const;
	void set_decoder_state(const decoder_state &state);

private:
	struct frame
	{
		std::vector<uint8_t> y, cb, cr;
		int temporal_reference = 0;
		uint32_t timecode = 0;
		int picture_type = 0;
		bool valid = false;
	};

	using vlc_node = mpeg1_detail::vlc_node;
	using vlc_node_u = mpeg1_detail::vlc_node_u;

	// --- bit reading over the input buffer ---
	bool have_bits(size_t count) const;
	uint32_t read_bits(int count);
	int read_bit();
	void align_to_byte();
	int read_vlc(const vlc_node *table);
	uint16_t read_vlc_uint(const vlc_node_u *table);

	// Finds the byte offset of the next 00 00 01 start code prefix at or after
	// `from`, or SIZE_MAX. Only searches whole bytes.
	size_t find_start_code(size_t from) const;
	// True when the next 23 bits are zero, i.e. a start code follows.
	bool at_start_code() const;

	void discard_consumed();

	bool decode_sequence_header();
	void decode_gop_header();
	bool decode_picture();
	void decode_slice(int vertical_position);
	void decode_macroblock();
	void decode_block(int block);
	void decode_motion_vector(int &dst, int motion, int r_size, int f);

	// Stands in every macroblock from the one after the last that was written
	// up to and including `last`, by predicting it from the reference picture.
	void conceal_through(int last);

	void predict_macroblock();
	void copy_macroblock(const frame &src, int motion_h, int motion_v, bool full_pel);
	void interpolate_macroblock(const frame &fwd, int fwd_h, int fwd_v, const frame &bwd, int bwd_h, int bwd_v);

	void allocate_frames();
	void idct(int *block);

	// --- input buffer ---
	std::vector<uint8_t> m_buffer;
	size_t m_read_pos = 0;      // byte offset of the next unconsumed byte
	size_t m_bit_pos = 0;       // absolute bit offset into m_buffer
	size_t m_buffer_limit = 512 * 1024;

	// --- sequence state ---
	bool m_has_sequence = false;
	bool m_sequence_ended = false;
	int m_width = 0, m_height = 0;
	int m_mb_width = 0, m_mb_height = 0;
	int m_luma_width = 0, m_luma_height = 0;
	int m_chroma_width = 0, m_chroma_height = 0;
	uint8_t m_frame_rate_index = 0;
	uint8_t m_intra_quant[64];
	uint8_t m_non_intra_quant[64];

	// --- picture state ---
	int m_picture_type = 0;
	int m_temporal_reference = 0;
	uint32_t m_gop_timecode = 0;
	int m_forward_f_code = 0, m_backward_f_code = 0;
	int m_forward_r_size = 0, m_backward_r_size = 0;
	int m_forward_f = 0, m_backward_f = 0;
	bool m_full_pel_forward = false, m_full_pel_backward = false;

	// --- slice / macroblock state ---
	int m_quantizer_scale = 0;
	int m_macroblock_address = 0;
	// Set while the next macroblock is the first of a slice, whose address
	// increment is measured from the slice's vertical position rather than from
	// the macroblock before it. See decode_macroblock().
	bool m_slice_begin = false;
	// Set when a symbol turns up that cannot occur in a well-formed stream,
	// which means the bit reader has lost the slice. The rest of the slice is
	// abandoned and its macroblocks are concealed instead of being decoded from
	// what is no longer the right bit position. Purely per-slice, so it is not
	// part of decoder_state and does not affect the save state size.
	bool m_slice_error = false;
	int m_mb_row = 0, m_mb_col = 0;
	int m_macroblock_type = 0;
	bool m_macroblock_intra = false;
	int m_dc_predictor[3] = { 0, 0, 0 };
	int m_motion_fw_h = 0, m_motion_fw_v = 0;
	int m_motion_bw_h = 0, m_motion_bw_v = 0;
	bool m_has_forward = false, m_has_backward = false;
	int m_block_data[64];

	// --- frames ---
	//
	// Pictures arrive in decode order but must be shown in display order, so
	// the two anchor pictures either side of any B pictures are both kept.
	// m_past is the older anchor, m_future the newer one, and m_work is the
	// picture currently being decoded. An anchor only becomes displayable once
	// the next anchor has been decoded.
	frame m_frames[3];
	frame *m_past = nullptr;
	frame *m_future = nullptr;
	frame *m_work = nullptr;
	frame *m_display_frame = nullptr;
};

#endif // MAME_MACHINE_MPEG1VIDEO_H
