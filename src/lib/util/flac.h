// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    flac.h

    FLAC decoding helper, backed by the rflac decoder from
    libretro-common.  The libFLAC-based encoder wrapper is gone with its
    last users (the CHD compression codecs); the sole remaining consumer
    of decoding is the samples device reading .flac sample files.

***************************************************************************/

#ifndef MAME_UTIL_FLAC_H
#define MAME_UTIL_FLAC_H

#pragma once

#include "utilfwd.h"

#include <cstdint>
#include <vector>

struct rflac_ctx;


// ======================> flac_decoder

class flac_decoder
{
public:
	// construction/destruction
	flac_decoder();
	flac_decoder(util::read_stream &file);
	~flac_decoder();

	// getters (valid after reset)
	uint32_t sample_rate() const { return m_sample_rate; }
	uint8_t channels() const { return m_channels; }
	uint8_t bits_per_sample() const { return m_bits_per_sample; }
	uint32_t total_samples() const { return m_total_samples; }

	// reset
	bool reset(util::read_stream &file);

	// decode to a buffer; num_samples counts frames per channel
	bool decode_interleaved(int16_t *samples, uint32_t num_samples, bool swap_endian = false);

	// finish up; returns bytes of compressed input consumed
	uint32_t finish();

private:
	bool parse_header();

	// internal state
	rflac_ctx *             m_decoder;          // actual decoder
	std::vector<uint8_t>    m_compressed;       // whole compressed stream
	uint32_t                m_consumed;         // bytes handed to the decoder

	// stream geometry from STREAMINFO
	uint32_t                m_sample_rate;
	uint8_t                 m_channels;
	uint8_t                 m_bits_per_sample;
	uint32_t                m_total_samples;
};

#endif // MAME_UTIL_FLAC_H
