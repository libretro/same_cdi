// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    flac.cpp

    FLAC decoding helper, backed by the rflac decoder from
    libretro-common.

***************************************************************************/

#include "flac.h"

#include "ioprocs.h"

#include <formats/rflac.h>

#include <cstring>


//**************************************************************************
//  FLAC DECODER
//**************************************************************************

flac_decoder::flac_decoder()
	: m_decoder(nullptr)
	, m_consumed(0)
	, m_sample_rate(0)
	, m_channels(0)
	, m_bits_per_sample(0)
	, m_total_samples(0)
{
}

flac_decoder::flac_decoder(util::read_stream &file)
	: flac_decoder()
{
	reset(file);
}

flac_decoder::~flac_decoder()
{
	if (m_decoder)
		rflac_free(m_decoder);
}

/**
 * @brief   Point the decoder at a stream and parse its header.
 */
bool flac_decoder::reset(util::read_stream &file)
{
	if (m_decoder)
	{
		rflac_free(m_decoder);
		m_decoder = nullptr;
	}
	m_consumed = 0;
	m_sample_rate = 0;
	m_channels = 0;
	m_bits_per_sample = 0;
	m_total_samples = 0;

	// slurp the stream; sample files are small and rflac is fed the
	// span whole, so the whole-file read replaces a pull callback
	m_compressed.clear();
	uint8_t chunk[16384];
	for (;;)
	{
		size_t actual = 0;
		std::error_condition err = file.read(chunk, sizeof(chunk), actual);
		if (err)
			return false;
		if (actual == 0)
			break;
		m_compressed.insert(m_compressed.end(), chunk, chunk + actual);
	}

	m_decoder = rflac_new();
	if (!m_decoder)
		return false;

	rflac_set_in(m_decoder, m_compressed.data(), m_compressed.size());
	rflac_set_eof(m_decoder);
	return parse_header();
}

/**
 * @brief   Run the decoder with no output until the geometry is known.
 */
bool flac_decoder::parse_header()
{
	while (!rflac_format(m_decoder))
	{
		size_t taken = 0;
		int result = rflac_process(m_decoder, &taken, nullptr);
		m_consumed += taken;
		if (result == RFLAC_PROCESS_ERROR)
			return false;
		if (result == RFLAC_PROCESS_END && !rflac_format(m_decoder))
			return false;
	}

	const rflac_format_t *fmt = rflac_format(m_decoder);
	m_sample_rate = fmt->sample_rate;
	m_channels = uint8_t(fmt->channels);
	m_bits_per_sample = uint8_t(fmt->bits_per_sample);
	m_total_samples = uint32_t(rflac_total_frames(m_decoder));
	return true;
}

/**
 * @brief   Decode num_samples frames per channel of interleaved PCM.
 */
bool flac_decoder::decode_interleaved(int16_t *samples, uint32_t num_samples, bool swap_endian)
{
	if (!m_decoder)
		return false;

	rflac_set_out_s16(m_decoder, samples, num_samples);
	size_t produced = 0;
	while (produced < num_samples)
	{
		size_t taken = 0, wrote = 0;
		int result = rflac_process(m_decoder, &taken, &wrote);
		m_consumed += taken;
		produced += wrote;
		if (result == RFLAC_PROCESS_ERROR)
			return false;
		if (result == RFLAC_PROCESS_END)
			break;
	}
	if (produced < num_samples)
		return false;

	if (swap_endian)
	{
		const size_t total = size_t(num_samples) * m_channels;
		for (size_t i = 0; i < total; i++)
			samples[i] = int16_t((uint16_t(samples[i]) << 8) | (uint16_t(samples[i]) >> 8));
	}
	return true;
}

/**
 * @brief   Report how much of the compressed stream was consumed.
 */
uint32_t flac_decoder::finish()
{
	return m_consumed;
}
