// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#include "wavwrite.h"

#include "corefile.h"

#include "osdcomm.h"

#include <new>
#include <string>
#include <vector>


namespace util {

struct wav_file
{
	util::core_file::ptr file;
	std::vector<std::int16_t> temp;
	std::uint32_t total_offs = 0U;
	std::uint32_t data_offs = 0U;
};


wav_file_ptr wav_open(std::string_view filename, int sample_rate, int channels)
{
	std::uint32_t temp32;
	std::uint16_t temp16;

	// allocate memory for the wav struct
	wav_file_ptr wav(new (std::nothrow) wav_file);
	if (!wav)
		return nullptr;

	// create the file
	if (util::core_file::open(std::string(filename), OPEN_FLAG_WRITE | OPEN_FLAG_CREATE, wav->file))
		wav->file.reset();
	if (!wav->file)
		return nullptr;

	// write the 'RIFF' header
	{ size_t actual; wav->file->write("RIFF", 4, actual); }

	// write the total size
	temp32 = 0;
	{ std::uint64_t pos = 0; wav->file->tell(pos); wav->total_offs = std::uint32_t(pos); }
	{ size_t actual; wav->file->write(&temp32, 4, actual); }

	// write the 'WAVE' type
	{ size_t actual; wav->file->write("WAVE", 4, actual); }

	// write the 'fmt ' tag
	{ size_t actual; wav->file->write("fmt ", 4, actual); }

	// write the format length
	temp32 = little_endianize_int32(16);
	{ size_t actual; wav->file->write(&temp32, 4, actual); }

	// write the format (PCM)
	temp16 = little_endianize_int16(1);
	{ size_t actual; wav->file->write(&temp16, 2, actual); }

	// write the channels
	temp16 = little_endianize_int16(channels);
	{ size_t actual; wav->file->write(&temp16, 2, actual); }

	// write the sample rate
	temp32 = little_endianize_int32(sample_rate);
	{ size_t actual; wav->file->write(&temp32, 4, actual); }

	// write the bytes/second
	std::uint32_t const bps = sample_rate * 2 * channels;
	temp32 = little_endianize_int32(bps);
	{ size_t actual; wav->file->write(&temp32, 4, actual); }

	// write the block align
	std::uint16_t const align = 2 * channels;
	temp16 = little_endianize_int16(align);
	{ size_t actual; wav->file->write(&temp16, 2, actual); }

	// write the bits/sample
	temp16 = little_endianize_int16(16);
	{ size_t actual; wav->file->write(&temp16, 2, actual); }

	// write the 'data' tag
	{ size_t actual; wav->file->write("data", 4, actual); }

	// write the data length
	temp32 = 0;
	{ std::uint64_t pos = 0; wav->file->tell(pos); wav->data_offs = std::uint32_t(pos); }
	{ size_t actual; wav->file->write(&temp32, 4, actual); }

	return wav;
}


void wav_close(wav_file *wav)
{
	if (!wav)
		return;

	if (wav->file)
	{
		std::uint32_t temp32;
		std::uint64_t pos = 0;
		wav->file->tell(pos);
		std::uint32_t const total = std::uint32_t(pos);

		// update the total file size
		wav->file->seek(wav->total_offs, SEEK_SET);
		temp32 = total - (wav->total_offs + 4);
		temp32 = little_endianize_int32(temp32);
		{ size_t actual; wav->file->write(&temp32, 4, actual); }

		// update the data size
		wav->file->seek(wav->data_offs, SEEK_SET);
		temp32 = total - (wav->data_offs + 4);
		temp32 = little_endianize_int32(temp32);
		{ size_t actual; wav->file->write(&temp32, 4, actual); }

		wav->file.reset();
	}

	delete wav;
}


void wav_add_data_16(wav_file &wav, int16_t *data, int samples)
{
	// just write and flush the data
	{ size_t actual; wav.file->write(data, size_t(2) * (samples), actual); }
	wav.file->flush();
}


void wav_add_data_32(wav_file &wav, int32_t *data, int samples, int shift)
{
	if (!samples)
		return;

	// resize dynamic array - don't want it to copy if it needs to expand
	wav.temp.clear();
	wav.temp.resize(samples);

	// clamp
	for (int i = 0; i < samples; i++)
	{
		int val = data[i] >> shift;
		wav.temp[i] = (val < -32768) ? -32768 : (val > 32767) ? 32767 : val;
	}

	// write and flush
	{ size_t actual; wav.file->write(&wav.temp[0], size_t(2) * (samples), actual); }
	wav.file->flush();
}


void wav_add_data_16lr(wav_file &wav, int16_t *left, int16_t *right, int samples)
{
	if (!samples)
		return;

	// resize dynamic array - don't want it to copy if it needs to expand
	wav.temp.clear();
	wav.temp.resize(samples * 2);

	// interleave
	for (int i = 0; i < samples * 2; i++)
		wav.temp[i] = (i & 1) ? right[i / 2] : left[i / 2];

	// write and flush
	{ size_t actual; wav.file->write(&wav.temp[0], size_t(4) * (samples), actual); }
	wav.file->flush();
}


void wav_add_data_32lr(wav_file &wav, int32_t *left, int32_t *right, int samples, int shift)
{
	if (!samples)
		return;

	// resize dynamic array - don't want it to copy if it needs to expand
	wav.temp.clear();
	wav.temp.resize(samples * 2);

	// interleave
	for (int i = 0; i < samples * 2; i++)
	{
		int val = (i & 1) ? right[i / 2] : left[i / 2];
		val >>= shift;
		wav.temp[i] = (val < -32768) ? -32768 : (val > 32767) ? 32767 : val;
	}

	// write and flush
	{ size_t actual; wav.file->write(&wav.temp[0], size_t(4) * (samples), actual); }
	wav.file->flush();
}

} // namespace util
