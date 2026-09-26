// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    sound.cpp

    Core sound functions and definitions.

***************************************************************************/

#include "emu.h"
#include "speaker.h"
#include "emuopts.h"
#include "osdepend.h"
#include "config.h"
#include "wavwrite.h"

//**************************************************************************
//  CONSTANTS
//**************************************************************************



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const attotime sound_manager::STREAMS_UPDATE_ATTOTIME = attotime::from_hz(STREAMS_UPDATE_FREQUENCY);



//**************************************************************************
//  STREAM BUFFER
//**************************************************************************

//-------------------------------------------------
//  stream_buffer - constructor
//-------------------------------------------------

stream_buffer::stream_buffer(u32 sample_rate) :
	m_end_second(0),
	m_end_sample(0),
	m_sample_rate(sample_rate),
	m_sample_attos((sample_rate == 0) ? ATTOSECONDS_PER_SECOND : ((ATTOSECONDS_PER_SECOND + sample_rate - 1) / sample_rate)),
	m_buffer(sample_rate)
{
}


//-------------------------------------------------
//  stream_buffer - destructor
//-------------------------------------------------

stream_buffer::~stream_buffer()
{
}


//-------------------------------------------------
//  set_sample_rate - set a new sample rate for
//  this buffer
//-------------------------------------------------

void stream_buffer::set_sample_rate(u32 rate, bool resample)
{
	// skip if nothing is actually changing
	if (rate == m_sample_rate)
		return;

	// force resampling off if coming to or from an invalid rate, or if we're at time 0 (startup)
	sound_assert(rate >= SAMPLE_RATE_MINIMUM - 1);
	if (rate < SAMPLE_RATE_MINIMUM || m_sample_rate < SAMPLE_RATE_MINIMUM || (m_end_second == 0 && m_end_sample == 0))
		resample = false;

	// note the time and period of the current buffer (end_time is AFTER the final sample)
	attotime prevperiod = sample_period();
	attotime prevend = end_time();

	// compute the time and period of the new buffer
	attotime newperiod = attotime(0, (ATTOSECONDS_PER_SECOND + rate - 1) / rate);
	attotime newend = attotime(prevend.seconds(), (prevend.attoseconds() / newperiod.attoseconds()) * newperiod.attoseconds());

	// buffer a short runway of previous samples; in order to support smooth
	// sample rate changes (needed by, e.g., Q*Bert's Votrax), we buffer a few
	// samples at the previous rate, and then reconstitute them resampled
	// (via simple point sampling) at the new rate. The litmus test is the
	// voice when jumping off the edge in Q*Bert; without this extra effort
	// it is crackly and/or glitchy at times
	sample_t buffer[64];
	int buffered_samples = std::min(m_sample_rate, std::min(rate, u32(std::size(buffer))));

	// if the new rate is lower, downsample into our holding buffer;
	// otherwise just copy into our holding buffer for later upsampling
	bool new_rate_higher = (rate > m_sample_rate);
	if (resample)
	{
		if (!new_rate_higher)
			backfill_downsample(&buffer[0], buffered_samples, newend, newperiod);
		else
		{
			u32 end = m_end_sample;
			for (int index = 0; index < buffered_samples; index++)
			{
				end = prev_index(end);
				buffer[index] = get(end);
			}
		}
	}

	// ensure our buffer is large enough to hold a full second at the new rate
	if (m_buffer.size() < rate)
		m_buffer.resize(rate);

	// set the new rate
	m_sample_rate = rate;
	m_sample_attos = newperiod.attoseconds();

	// compute the new end sample index based on the buffer time
	m_end_sample = time_to_buffer_index(prevend, false, true);

	// if the new rate is higher, upsample from our temporary buffer;
	// otherwise just copy our previously-downsampled data
	if (resample)
	{
		if (new_rate_higher)
			backfill_upsample(&buffer[0], buffered_samples, prevend, prevperiod);
		else
		{
			u32 end = m_end_sample;
			for (int index = 0; index < buffered_samples; index++)
			{
				end = prev_index(end);
				put(end, buffer[index]);
			}
		}
	}

	// if not resampling, clear the buffer
	else
		fill(0);
}

//-------------------------------------------------
//  index_time - return the attotime of a given
//  index within the buffer
//-------------------------------------------------

attotime stream_buffer::index_time(s32 index) const
{
	index = clamp_index(index);
	return attotime(m_end_second - ((index > m_end_sample) ? 1 : 0), index * m_sample_attos);
}


//-------------------------------------------------
//  time_to_buffer_index - given an attotime,
//  return the buffer index corresponding to it
//-------------------------------------------------

u32 stream_buffer::time_to_buffer_index(attotime time, bool round_up, bool allow_expansion)
{
	// compute the sample index within the second
	int sample = (time.attoseconds() + (round_up ? (m_sample_attos - 1) : 0)) / m_sample_attos;
	sound_assert(sample >= 0 && sample <= size());

	// if the time is past the current end, make it the end
	if (time.seconds() > m_end_second || (time.seconds() == m_end_second && sample > m_end_sample))
	{
		sound_assert(allow_expansion);

		m_end_sample = sample;
		m_end_second = time.m_seconds;

		// due to round_up, we could tweak over the line into the next second
		if (sample >= size())
		{
			m_end_sample -= size();
			m_end_second++;
		}
	}

	// if the time is before the start, fail
	if (time.seconds() + 1 < m_end_second || (time.seconds() + 1 == m_end_second && sample < m_end_sample))
		throw emu_fatalerror("Attempt to create an out-of-bounds view");

	return clamp_index(sample);
}


//-------------------------------------------------
//  backfill_downsample - this is called BEFORE
//  the sample rate change to downsample from the
//  end of the current buffer into a temporary
//  holding location
//-------------------------------------------------

void stream_buffer::backfill_downsample(sample_t *dest, int samples, attotime newend, attotime newperiod)
{
	// compute the time of the first sample to be backfilled; start one period before
	attotime time = newend - newperiod;

	// loop until we run out of buffered data
	int dstindex;
	for (dstindex = 0; dstindex < samples && time.seconds() >= 0; dstindex++)
	{
		u32 srcindex   = time_to_buffer_index(time, false);
		dest[dstindex] = get(srcindex);
		time -= newperiod;
	}
	for ( ; dstindex < samples; dstindex++)
		dest[dstindex] = 0;
}


//-------------------------------------------------
//  backfill_upsample - this is called AFTER the
//  sample rate change to take a copied buffer
//  of samples at the old rate and upsample them
//  to the new (current) rate
//-------------------------------------------------

void stream_buffer::backfill_upsample(sample_t const *src, int samples, attotime prevend, attotime prevperiod)
{
	// compute the time of the first sample to be backfilled; start one period before
	attotime time = end_time() - sample_period();

	// also adjust the buffered sample end time to point to the sample time of the
	// final sample captured
	prevend -= prevperiod;

	// loop until we run out of buffered data
	u32 end = m_end_sample;
	int srcindex = 0;
	while (1)
	{
		// if our backfill time is before the current buffered sample time,
		// back up until we have a sample that covers this time
		while (time < prevend && srcindex < samples)
		{
			prevend -= prevperiod;
			srcindex++;
		}

		// stop when we run out of source
		if (srcindex >= samples)
			break;

		// write this sample at the pevious position
		end = prev_index(end);
		put(end, src[srcindex]);

		// back up to the next sample time
		time -= sample_period();
	}
}



//**************************************************************************
//  SOUND STREAM OUTPUT
//**************************************************************************

//-------------------------------------------------
//  sound_stream_output - constructor
//-------------------------------------------------

sound_stream_output::sound_stream_output() :
	m_stream(nullptr),
	m_index(0),
	m_gain(1.0)
{
}


//-------------------------------------------------
//  init - initialization
//-------------------------------------------------

void sound_stream_output::init(sound_stream &stream, u32 index, char const *tag)
{
	// set the passed-in data
	m_stream = &stream;
	m_index = index;

	// save our state
	auto &save = stream.device().machine().save();
	save.save_item(&stream.device(), "stream.output", tag, index, NAME(m_gain));
}


//-------------------------------------------------
//  name - return the friendly name of this output
//-------------------------------------------------

std::string sound_stream_output::name() const
{
	// start with our owning stream's name
	std::ostringstream str;
	util::stream_format(str, "%s Ch.%d", m_stream->name(), m_stream->output_base() + m_index);
	return str.str();
}


//-------------------------------------------------
//  optimize_resampler - optimize resamplers by
//  either returning the native rate or another
//  input's resampler if they can be reused
//-------------------------------------------------

sound_stream_output &sound_stream_output::optimize_resampler(sound_stream_output *input_resampler)
{
	// if no resampler, or if the resampler rate matches our rate, return ourself
	if (input_resampler == nullptr || buffer_sample_rate() == input_resampler->buffer_sample_rate())
		return *this;

	// scan our list of resamplers to see if there's another match
	for (auto &resampler : m_resampler_list)
		if (resampler->buffer_sample_rate() == input_resampler->buffer_sample_rate())
			return *resampler;

	// add the input to our list and return the one we were given back
	m_resampler_list.push_back(input_resampler);
	return *input_resampler;
}



//**************************************************************************
//  SOUND STREAM INPUT
//**************************************************************************

//-------------------------------------------------
//  sound_stream_input - constructor
//-------------------------------------------------

sound_stream_input::sound_stream_input() :
	m_owner(nullptr),
	m_native_source(nullptr),
	m_resampler_source(nullptr),
	m_index(0),
	m_gain(1.0),
	m_user_gain(1.0)
{
}


//-------------------------------------------------
//  init - initialization
//-------------------------------------------------

void sound_stream_input::init(sound_stream &stream, u32 index, char const *tag, sound_stream_output *resampler)
{
	// set the passed-in values
	m_owner = &stream;
	m_index = index;
	m_resampler_source = resampler;

	// save our state
	auto &save = stream.device().machine().save();
	save.save_item(&stream.device(), "stream.input", tag, index, NAME(m_gain));
	save.save_item(&stream.device(), "stream.input", tag, index, NAME(m_user_gain));
}


//-------------------------------------------------
//  name - return the friendly name of this input
//-------------------------------------------------

std::string sound_stream_input::name() const
{
	// start with our owning stream's name
	std::ostringstream str;
	util::stream_format(str, "%s", m_owner->name());

	// if we have a source, indicate where the sound comes from by device name and tag
	if (valid())
		util::stream_format(str, " <- %s", m_native_source->name());
	return str.str();
}


//-------------------------------------------------
//  set_source - wire up the output source for
//  our consumption
//-------------------------------------------------

void sound_stream_input::set_source(sound_stream_output *source)
{
	m_native_source = source;
	if (m_resampler_source != nullptr)
		m_resampler_source->stream().set_input(0, &source->stream(), source->index());
}


//-------------------------------------------------
//  update - update our source's stream to the
//  current end time and return a view to its
//  contents
//-------------------------------------------------

read_stream_view sound_stream_input::update(attotime start, attotime end)
{
	// shouldn't get here unless valid
	sound_assert(valid());

	// pick an optimized resampler
	sound_stream_output &source = m_native_source->optimize_resampler(m_resampler_source);

	// if not using our own resampler, keep it up to date in case we need to invoke it later
	if (m_resampler_source != nullptr && &source != m_resampler_source)
		m_resampler_source->set_end_time(end);

	// update the source, returning a view of the needed output over the start and end times
	return source.stream().update_view(start, end, source.index()).apply_gain(m_gain * m_user_gain * source.gain());
}


//-------------------------------------------------
//  apply_sample_rate_changes - tell our sources
//  to apply any sample rate changes, informing
//  them of our current rate
//-------------------------------------------------

void sound_stream_input::apply_sample_rate_changes(u32 updatenum, u32 downstream_rate)
{
	// shouldn't get here unless valid
	sound_assert(valid());

	// if we have a resampler, tell it (and it will tell the native source)
	if (m_resampler_source != nullptr)
		m_resampler_source->stream().apply_sample_rate_changes(updatenum, downstream_rate);

	// otherwise, just tell the native source directly
	else
		m_native_source->stream().apply_sample_rate_changes(updatenum, downstream_rate);
}



//**************************************************************************
//  SOUND STREAM
//**************************************************************************

//-------------------------------------------------
//  sound_stream - private common constructor
//-------------------------------------------------

sound_stream::sound_stream(device_t &device, u32 inputs, u32 outputs, u32 output_base, u32 sample_rate, sound_stream_flags flags) :
	m_device(device),
	m_next(nullptr),
	m_sample_rate((sample_rate < SAMPLE_RATE_MINIMUM) ? (SAMPLE_RATE_MINIMUM - 1) : (sample_rate < SAMPLE_RATE_OUTPUT_ADAPTIVE) ? sample_rate : 48000),
	m_pending_sample_rate(SAMPLE_RATE_INVALID),
	m_last_sample_rate_update(0),
	m_input_adaptive(sample_rate == SAMPLE_RATE_INPUT_ADAPTIVE),
	m_output_adaptive(sample_rate == SAMPLE_RATE_OUTPUT_ADAPTIVE),
	m_synchronous((flags & STREAM_SYNCHRONOUS) != 0),
	m_resampling_disabled((flags & STREAM_DISABLE_INPUT_RESAMPLING) != 0),
	m_sync_timer(nullptr),
	m_input(inputs),
	m_input_view(inputs),
	m_empty_buffer(100),
	m_output_base(output_base),
	m_output(outputs),
	m_output_view(outputs)
{
	sound_assert(outputs > 0);

	// create a name
	m_name = m_device.name();
	m_name += " '";
	m_name += m_device.tag();
	m_name += "'";

	// create a unique tag for saving
	std::string state_tag = string_format("%d", m_device.machine().sound().unique_id());
	auto &save = m_device.machine().save();
	save.save_item(&m_device, "stream.sample_rate", state_tag.c_str(), 0, NAME(m_sample_rate));
	save.register_postload(save_prepost_delegate(FUNC(sound_stream::postload), this));

	// initialize all inputs
	for (unsigned int inputnum = 0; inputnum < m_input.size(); inputnum++)
	{
		// allocate a resampler stream if needed, and get a pointer to its output
		sound_stream_output *resampler = nullptr;
		if (!m_resampling_disabled)
		{
			m_resampler_list.push_back(std::make_unique<default_resampler_stream>(m_device));
			resampler = &m_resampler_list.back()->m_output[0];
		}

		// add the new input
		m_input[inputnum].init(*this, inputnum, state_tag.c_str(), resampler);
	}

	// initialize all outputs
	for (unsigned int outputnum = 0; outputnum < m_output.size(); outputnum++)
		m_output[outputnum].init(*this, outputnum, state_tag.c_str());

	// create an update timer for synchronous streams
	if (synchronous())
		m_sync_timer = m_device.machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(sound_stream::sync_update), this));

	// force an update to the sample rates
	sample_rate_changed();
}


//-------------------------------------------------
//  sound_stream - constructor
//-------------------------------------------------

sound_stream::sound_stream(device_t &device, u32 inputs, u32 outputs, u32 output_base, u32 sample_rate, stream_update_delegate callback, sound_stream_flags flags) :
	sound_stream(device, inputs, outputs, output_base, sample_rate, flags)
{
	m_callback_ex = std::move(callback);
}


//-------------------------------------------------
//  ~sound_stream - destructor
//-------------------------------------------------

sound_stream::~sound_stream()
{
}


//-------------------------------------------------
//  set_sample_rate - set the sample rate on a
//  given stream
//-------------------------------------------------

void sound_stream::set_sample_rate(u32 new_rate)
{
	// we will update this on the next global update
	if (new_rate != sample_rate())
		m_pending_sample_rate = new_rate;
}


//-------------------------------------------------
//  set_input - configure a stream's input
//-------------------------------------------------

void sound_stream::set_input(int index, sound_stream *input_stream, int output_index, float gain)
{
	// make sure it's a valid input
	if (index >= m_input.size())
		fatalerror("stream_set_input attempted to configure nonexistent input %d (%d max)\n", index, int(m_input.size()));

	// make sure it's a valid output
	if (input_stream != nullptr && output_index >= input_stream->m_output.size())
		fatalerror("stream_set_input attempted to use a nonexistent output %d (%d max)\n", output_index, int(m_output.size()));

	// wire it up
	m_input[index].set_source((input_stream != nullptr) ? &input_stream->m_output[output_index] : nullptr);
	m_input[index].set_gain(gain);

	// update sample rates now that we know the input
	sample_rate_changed();
}


//-------------------------------------------------
//  update - force a stream to update to
//  the current emulated time
//-------------------------------------------------

void sound_stream::update()
{
	// ignore any update requests if we're already up to date
	attotime start = m_output[0].end_time();
	attotime end = m_device.machine().time();
	if (start >= end)
		return;

	// regular update then
	update_view(start, end);
}


//-------------------------------------------------
//  update_view - force a stream to update to
//  the current emulated time and return a view
//  to the generated samples from the given
//  output number
//-------------------------------------------------

read_stream_view sound_stream::update_view(attotime start, attotime end, u32 outputnum)
{
	sound_assert(start <= end);
	sound_assert(outputnum < m_output.size());

	// clean up parameters for when the asserts go away
	if (outputnum >= m_output.size())
		outputnum = 0;
	if (start > end)
		start = end;

	// reposition our start to coincide with the current buffer end
	attotime update_start = m_output[outputnum].end_time();
	if (update_start <= end)
	{
		// create views for all the outputs
		for (unsigned int outindex = 0; outindex < m_output.size(); outindex++)
			m_output_view[outindex] = m_output[outindex].view(update_start, end);

		// skip if nothing to do
		u32 samples = m_output_view[0].samples();
		sound_assert(samples >= 0);
		if (samples != 0 && m_sample_rate >= SAMPLE_RATE_MINIMUM)
		{
			sound_assert(!synchronous() || samples == 1);

			// ensure all input streams are up to date, and create views for them as well
			for (unsigned int inputnum = 0; inputnum < m_input.size(); inputnum++)
			{
				if (m_input[inputnum].valid())
					m_input_view[inputnum] = m_input[inputnum].update(update_start, end);
				else
					m_input_view[inputnum] = empty_view(update_start, end);
				sound_assert(m_input_view[inputnum].samples() > 0);
				sound_assert(m_resampling_disabled || m_input_view[inputnum].sample_rate() == m_sample_rate);
			}


			// if we have an extended callback, that's all we need
			m_callback_ex(*this, m_input_view, m_output_view);

		}
	}

	// return the requested view
	return read_stream_view(m_output_view[outputnum], start);
}


//-------------------------------------------------
//  apply_sample_rate_changes - if there is a
//  pending sample rate change, apply it now
//-------------------------------------------------

void sound_stream::apply_sample_rate_changes(u32 updatenum, u32 downstream_rate)
{
	// grab the new rate and invalidate
	u32 new_rate = (m_pending_sample_rate != SAMPLE_RATE_INVALID) ? m_pending_sample_rate : m_sample_rate;
	m_pending_sample_rate = SAMPLE_RATE_INVALID;

	// clamp to the minimum - 1 (anything below minimum means "off" and
	// will not call the sound callback at all)
	if (new_rate < SAMPLE_RATE_MINIMUM)
		new_rate = SAMPLE_RATE_MINIMUM - 1;

	// if we're input adaptive, override with the rate of our input
	if (input_adaptive() && m_input.size() > 0 && m_input[0].valid())
		new_rate = m_input[0].source().stream().sample_rate();

	// if we're output adaptive, override with the rate of our output
	if (output_adaptive())
	{
		if (m_last_sample_rate_update == updatenum)
			sound_assert(new_rate == m_sample_rate);
		else
			m_last_sample_rate_update = updatenum;
		new_rate = downstream_rate;
	}

	// if something is different, process the change
	if (new_rate != SAMPLE_RATE_INVALID && new_rate != m_sample_rate)
	{
		// update to the new rate and notify everyone
		m_sample_rate = new_rate;
		sample_rate_changed();
	}

	// now call through our inputs and apply the rate change there
	for (auto &input : m_input)
		if (input.valid())
			input.apply_sample_rate_changes(updatenum, m_sample_rate);
}

//-------------------------------------------------
//  sample_rate_changed - recompute sample
//  rate data, and all streams that are affected
//  by this stream
//-------------------------------------------------

void sound_stream::sample_rate_changed()
{
	// if invalid, just punt
	if (m_sample_rate == SAMPLE_RATE_INVALID)
		return;

	// update all output buffers
	for (auto &output : m_output)
		output.sample_rate_changed(m_sample_rate);

	// if synchronous, prime the timer
	if (synchronous())
		reprime_sync_timer();
}


//-------------------------------------------------
//  postload - save/restore callback
//-------------------------------------------------

void sound_stream::postload()
{
	// Resume from where the sound manager will next ask for samples, which is
	// not necessarily now.
	//
	// A save state is written at the end of an emulated video frame, and that
	// lands a hair *before* the periodic sound update covering the frame, so
	// sound_manager::m_last_update is a whole update period behind
	// machine().time() in essentially every state. Setting the buffer end to
	// now claims that period has already been generated, and the next
	// sound_manager::update() then mixes it out of whatever the buffer still
	// happens to hold rather than asking the streams to produce it.
	//
	// That is one update's worth of stale audio per load. A state loaded by
	// hand costs a single 20ms glitch, but rewind and run-ahead load one every
	// frame, so *every* sample reaching the frontend is stale - and since the
	// buffer is a one-second ring indexed by time, rewinding walks back over
	// the same ring positions and replays the second before the rewind began,
	// over and over.
	attotime resume = m_device.machine().sound().last_update();
	if (resume > m_device.machine().time())
		resume = m_device.machine().time();
	for (auto &output : m_output)
		output.set_end_time_rounded_up(resume);

	// recompute the sample rate information
	sample_rate_changed();
}


//-------------------------------------------------
//  reprime_sync_timer - set up the next sync
//  timer to go off just a hair after the end of
//  the current sample period
//-------------------------------------------------

void sound_stream::reprime_sync_timer()
{
	attotime curtime = m_device.machine().time();
	attotime target = m_output[0].end_time() + attotime(0, 1);
	m_sync_timer->adjust(target - curtime);
}


//-------------------------------------------------
//  sync_update - timer callback to handle a
//  synchronous stream
//-------------------------------------------------

void sound_stream::sync_update(void *, s32)
{
	update();
	reprime_sync_timer();
}


//-------------------------------------------------
//  empty_view - return an empty view covering the
//  given time period as a substitute for invalid
//  inputs
//-------------------------------------------------

read_stream_view sound_stream::empty_view(attotime start, attotime end)
{
	// if our dummy buffer doesn't match our sample rate, update and clear it
	if (m_empty_buffer.sample_rate() != m_sample_rate)
		m_empty_buffer.set_sample_rate(m_sample_rate, false);

	// allocate a write view so that it can expand, and convert back to a read view
	// on the return
	return write_stream_view(m_empty_buffer, start, end);
}



//**************************************************************************
//  RESAMPLER STREAM
//**************************************************************************

//-------------------------------------------------
//  default_resampler_stream - derived sound_stream
//  class that handles resampling
//-------------------------------------------------

default_resampler_stream::default_resampler_stream(device_t &device) :
	sound_stream(device, 1, 1, 0, SAMPLE_RATE_OUTPUT_ADAPTIVE, stream_update_delegate(&default_resampler_stream::resampler_sound_update, this), STREAM_DISABLE_INPUT_RESAMPLING),
	m_max_latency(0)
{
	// create a name
	m_name = "Default Resampler '";
	m_name += device.tag();
	m_name += "'";
}


//-------------------------------------------------
//  resampler_sound_update - stream callback
//  handler for resampling an input stream to the
//  target sample rate of the output
//-------------------------------------------------

void default_resampler_stream::resampler_sound_update(sound_stream &stream, std::vector<read_stream_view> const &inputs, std::vector<write_stream_view> &outputs)
{
	sound_assert(inputs.size() == 1);
	sound_assert(outputs.size() == 1);

	auto &input = inputs[0];
	auto &output = outputs[0];

	// if the input has an invalid rate, just fill with zeros
	if (input.sample_rate() <= 1)
	{
		output.fill(0);
		return;
	}

	// optimize_resampler ensures we should not have equal sample rates
	sound_assert(input.sample_rate() != output.sample_rate());

	// compute the stepping value and the inverse
	stream_buffer::sample_t step = stream_buffer::sample_t(input.sample_rate()) / stream_buffer::sample_t(output.sample_rate());
	stream_buffer::sample_t stepinv = 1.0 / step;

	// determine the latency we need to introduce, in input samples:
	//    1 input sample for undersampled inputs
	//    1 + step input samples for oversampled inputs
	s64 latency_samples = 1 + ((step < 1.0) ? 0 : s32(step));
	if (latency_samples <= m_max_latency)
		latency_samples = m_max_latency;
	else
		m_max_latency = latency_samples;
	attotime latency = latency_samples * input.sample_period();

	// clamp the latency to the start (only relevant at the beginning)
	s32 dstindex = 0;
	attotime output_start = output.start_time();
	auto numsamples = output.samples();
	while (latency > output_start && dstindex < numsamples)
	{
		output.put(dstindex++, 0);
		output_start += output.sample_period();
	}
	if (dstindex >= numsamples)
		return;

	// create a rebased input buffer around the adjusted start time
	read_stream_view rebased(input, output_start - latency);
	sound_assert(rebased.start_time() + latency <= output_start);

	// compute the fractional input start position
	attotime delta = output_start - (rebased.start_time() + latency);
	sound_assert(delta.seconds() == 0);
	stream_buffer::sample_t srcpos = stream_buffer::sample_t(double(delta.attoseconds()) / double(rebased.sample_period_attoseconds()));
	sound_assert(srcpos <= 1.0f);

	// input is undersampled: point sample except where our sample period covers a boundary
	s32 srcindex = 0;
	if (step < 1.0)
	{
		stream_buffer::sample_t cursample = rebased.get(srcindex++);
		for ( ; dstindex < numsamples; dstindex++)
		{
			// if still within the current sample, just replicate
			srcpos += step;
			if (srcpos <= 1.0)
				output.put(dstindex, cursample);

			// if crossing a sample boundary, blend with the neighbor
			else
			{
				srcpos -= 1.0;
				sound_assert(srcpos <= step + 1e-5);
				stream_buffer::sample_t prevsample = cursample;
				cursample = rebased.get(srcindex++);
				output.put(dstindex, stepinv * (prevsample * (step - srcpos) + srcpos * cursample));
			}
		}
		sound_assert(srcindex <= rebased.samples());
	}

	// input is oversampled: sum the energy
	else
	{
		float cursample = rebased.get(srcindex++);
		for ( ; dstindex < numsamples; dstindex++)
		{
			// compute the partial first sample and advance
			stream_buffer::sample_t scale = 1.0 - srcpos;
			stream_buffer::sample_t sample = cursample * scale;

			// add in complete samples until we only have a fraction left
			stream_buffer::sample_t remaining = step - scale;
			while (remaining >= 1.0)
			{
				sample += rebased.get(srcindex++);
				remaining -= 1.0;
			}

			// add in the final partial sample
			cursample = rebased.get(srcindex++);
			sample += cursample * remaining;
			output.put(dstindex, sample * stepinv);

			// our position is now the remainder
			srcpos = remaining;
			sound_assert(srcindex <= rebased.samples());
		}
	}
}



//**************************************************************************
//  SOUND MANAGER
//**************************************************************************

// The CD-i streams run at up to 48 kHz. Keep 4096 samples per output: more
// than four mixer periods, including the resampler's look-behind. Fixed slots
// keep retro_serialize_size stable when CDDA/XA/MPEG change sample rates.
// Unlike resetting all outputs to m_last_update, this preserves samples that
// a DMA flush generated ahead of the mixer and the exact device read cursor.
namespace {
constexpr u32 AUDIO_STATE_SAMPLES = 4096;
constexpr u32 AUDIO_STATE_MAX_RATE = 48000;
}

std::vector<sound_stream *> sound_manager::state_streams() const
{
	std::vector<sound_stream *> result;
	std::function<void (sound_stream *)> add = [&](sound_stream *stream)
	{
		result.push_back(stream);
		for (auto &resampler : stream->m_resampler_list)
			add(resampler.get());
	};
	for (auto &stream : m_stream_list)
		add(stream.get());
	return result;
}

size_t sound_manager::state_size() const
{
	size_t size = 32;
	for (auto *stream : state_streams())
		size += 24 + stream->m_output.size() * (16 + AUDIO_STATE_SAMPLES * sizeof(float));
	return size;
}

bool sound_manager::write_state(void *data, size_t size)
{
	return transfer_state(data, size, false, false);
}

bool sound_manager::read_state(const void *data, size_t size, bool validate_only)
{
	return transfer_state(const_cast<void *>(data), size, true, validate_only);
}

bool sound_manager::transfer_state(void *data, size_t size, bool load, bool validate_only)
{
	if (size != state_size())
		return false;
	u8 *ptr = static_cast<u8 *>(data);
	// All trailer words (including float bit patterns) are little endian.
	auto word = [&](u32 value)
	{
		u32 result;
		if (load)
		{
			memcpy(&result, ptr, 4);
			result = little_endianize_int32(result);
		}
		else
		{
			result = little_endianize_int32(value);
			memcpy(ptr, &result, 4);
			result = value;
		}
		ptr += 4;
		return result;
	};
	auto scalar = [&](auto &value)
	{
		static_assert(sizeof(value) == 4, "audio state word");
		u32 bits;
		memcpy(&bits, &value, 4);
		bits = word(bits);
		if (load && !validate_only)
			memcpy(&value, &bits, 4);
	};
	auto streams = state_streams();
	if (word(0x53445541) != 0x53445541 || word(1) != 1 || word(streams.size()) != streams.size())
		return false;
	scalar(m_update_number);
	scalar(m_finalmix_leftover);
	scalar(m_compressor_scale);
	scalar(m_compressor_counter);
	word(0);
	for (auto *stream : streams)
	{
		u32 rate = word(stream->m_sample_rate);
		u32 pending = word(stream->m_pending_sample_rate);
		if (rate < SAMPLE_RATE_MINIMUM - 1 || rate > AUDIO_STATE_MAX_RATE ||
			(pending != SAMPLE_RATE_INVALID && pending > AUDIO_STATE_MAX_RATE))
			return false;
		if (load && !validate_only)
		{
			stream->m_sample_rate = rate;
			stream->m_pending_sample_rate = pending;
		}
		scalar(stream->m_last_sample_rate_update);
		if (word(stream->m_output.size()) != stream->m_output.size())
			return false;
		auto *resampler = dynamic_cast<default_resampler_stream *>(stream);
		u64 latency = resampler ? u64(resampler->m_max_latency) : 0;
		u32 low = word(u32(latency)), high = word(u32(latency >> 32));
		if (high || low > AUDIO_STATE_SAMPLES / 2)
			return false;
		if (load && !validate_only && resampler)
			resampler->m_max_latency = low;
		for (auto &output : stream->m_output)
		{
			auto &buffer = output.m_buffer;
			u32 buffer_rate = word(buffer.m_sample_rate);
			u32 second = word(u32(buffer.m_end_second));
			u32 end = word(buffer.m_end_sample);
			word(0);
			if (buffer_rate != rate || end >= buffer_rate)
				return false;
			if (load && !validate_only)
			{
				buffer.set_sample_rate(buffer_rate, false);
				buffer.m_end_second = s32(second);
				buffer.m_end_sample = end;
				output.m_resampler_list.clear();
			}
			// Store newest first; unused slots are zero, never stale ring data.
			u32 index = end;
			for (u32 i = 0; i < AUDIO_STATE_SAMPLES; ++i)
			{
				index = index ? index - 1 : buffer_rate - 1;
				float sample = (!load && i < buffer_rate) ? buffer.get(index) : 0;
				scalar(sample);
				if (load && !validate_only && i < buffer_rate)
					buffer.put(index, sample);
			}
		}
	}
	return true;
}

//-------------------------------------------------
//  sound_manager - constructor
//-------------------------------------------------

sound_manager::sound_manager(running_machine &machine) :
	m_machine(machine),
	m_update_timer(nullptr),
	m_update_number(0),
	m_last_update(attotime::zero),
	m_finalmix_leftover(0),
	m_samples_this_update(0),
	m_finalmix(machine.sample_rate()),
	m_leftmix(machine.sample_rate()),
	m_rightmix(machine.sample_rate()),
	m_compressor_scale(1.0),
	m_compressor_counter(0),
	m_compressor_enabled(machine.options().compressor()),
	m_muted(0),
	m_nosound_mode(machine.osd().no_sound()),
	m_attenuation(0),
	m_unique_id(0),
	m_wavfile(),
	m_first_reset(true)
{
	// get filename for WAV file or AVI file if specified
	const char *wavfile = machine.options().wav_write();
	const char *avifile = machine.options().avi_write();

	// handle -nosound and lower sample rate if not recording WAV or AVI
	if (m_nosound_mode && wavfile[0] == 0 && avifile[0] == 0)
		machine.m_sample_rate = 11025;

	// register callbacks
	machine.configuration().config_register(
			"mixer",
			configuration_manager::load_delegate(&sound_manager::config_load, this),
			configuration_manager::save_delegate(&sound_manager::config_save, this));
	machine.add_notifier(MACHINE_NOTIFY_PAUSE, machine_notify_delegate(&sound_manager::pause, this));
	machine.add_notifier(MACHINE_NOTIFY_RESUME, machine_notify_delegate(&sound_manager::resume, this));
	machine.add_notifier(MACHINE_NOTIFY_RESET, machine_notify_delegate(&sound_manager::reset, this));
	machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(&sound_manager::stop_recording, this));

	// register global states
	machine.save().save_item(NAME(m_last_update));

	// set the starting attenuation
	set_attenuation(machine.options().volume());

	// start the periodic update flushing timer
	m_update_timer = machine.scheduler().timer_alloc(timer_expired_delegate(FUNC(sound_manager::update), this));
	m_update_timer->adjust(STREAMS_UPDATE_ATTOTIME, 0, STREAMS_UPDATE_ATTOTIME);
}


//-------------------------------------------------
//  sound_manager - destructor
//-------------------------------------------------

sound_manager::~sound_manager()
{
}


//-------------------------------------------------
//  stream_alloc - allocate a new stream with the
//  new-style callback and flags
//-------------------------------------------------

sound_stream *sound_manager::stream_alloc(device_t &device, u32 inputs, u32 outputs, u32 sample_rate, stream_update_delegate callback, sound_stream_flags flags)
{
	// determine output base
	u32 output_base = 0;
	for (auto &stream : m_stream_list)
		if (&stream->device() == &device)
			output_base += stream->output_count();

	m_stream_list.push_back(std::make_unique<sound_stream>(device, inputs, outputs, output_base, sample_rate, callback, flags));
	return m_stream_list.back().get();
}


//-------------------------------------------------
//  start_recording - begin audio recording
//-------------------------------------------------

bool sound_manager::start_recording(std::string_view filename)
{
	if (m_wavfile)
		return false;
	m_wavfile = util::wav_open(filename, machine().sample_rate(), 2);
	return bool(m_wavfile);
}

bool sound_manager::start_recording()
{
	// open the output WAV file if specified
	char const *const filename = machine().options().wav_write();
	return *filename ? start_recording(filename) : false;
}


//-------------------------------------------------
//  stop_recording - end audio recording
//-------------------------------------------------

void sound_manager::stop_recording()
{
	// close any open WAV file
	m_wavfile.reset();
}


//-------------------------------------------------
//  set_attenuation - set the global volume
//-------------------------------------------------

void sound_manager::set_attenuation(float attenuation)
{
	// currently OSD only supports integral attenuation
	m_attenuation = int(attenuation);
	machine().osd().set_mastervolume(m_muted ? -32 : m_attenuation);
}


//-------------------------------------------------
//  indexed_mixer_input - return the mixer
//  device and input index of the global mixer
//  input
//-------------------------------------------------

bool sound_manager::indexed_mixer_input(int index, mixer_input &info) const
{
	// scan through the mixers until we find the indexed input
	for (device_mixer_interface &mixer : mixer_interface_enumerator(machine().root_device()))
	{
		if (index < mixer.inputs())
		{
			info.mixer = &mixer;
			info.stream = mixer.input_to_stream_input(index, info.inputnum);
			sound_assert(info.stream != nullptr);
			return true;
		}
		index -= mixer.inputs();
	}

	// didn't locate
	info.mixer = nullptr;
	return false;
}


//-------------------------------------------------
//  samples - fills the specified buffer with
//  16-bit stereo audio samples generated during
//  the current frame
//-------------------------------------------------

void sound_manager::samples(s16 *buffer)
{
	for (int sample = 0; sample < m_samples_this_update * 2; sample++)
		*buffer++ = m_finalmix[sample];
}


//-------------------------------------------------
//  mute - mute sound output
//-------------------------------------------------

void sound_manager::mute(bool mute, u8 reason)
{
	bool old_muted = m_muted;
	if (mute)
		m_muted |= reason;
	else
		m_muted &= ~reason;

	if(old_muted != (m_muted != 0))
		set_attenuation(m_attenuation);
}


//-------------------------------------------------
//  recursive_remove_stream_from_orphan_list -
//  remove the given stream from the orphan list
//  and recursively remove all our inputs
//-------------------------------------------------

void sound_manager::recursive_remove_stream_from_orphan_list(sound_stream *which)
{
	m_orphan_stream_list.erase(which);
	for (int inputnum = 0; inputnum < which->input_count(); inputnum++)
	{
		auto &input = which->input(inputnum);
		if (input.valid())
			recursive_remove_stream_from_orphan_list(&input.source().stream());
	}
}


//-------------------------------------------------
//  apply_sample_rate_changes - recursively
//  update sample rates throughout the system
//-------------------------------------------------

void sound_manager::apply_sample_rate_changes()
{
	// update sample rates if they have changed
	for (speaker_device &speaker : speaker_device_enumerator(machine().root_device()))
	{
		int stream_out;
		sound_stream *stream = speaker.output_to_stream_output(0, stream_out);

		// due to device removal, some speakers may end up with no outputs; just skip those
		if (stream != nullptr)
		{
			sound_assert(speaker.outputs() == 1);
			stream->apply_sample_rate_changes(m_update_number, machine().sample_rate());
		}
	}
}


//-------------------------------------------------
//  reset - reset all sound chips
//-------------------------------------------------

void sound_manager::reset()
{
	// reset all the sound chips
	for (device_sound_interface &sound : sound_interface_enumerator(machine().root_device()))
		sound.device().reset();

	// apply any sample rate changes now
	apply_sample_rate_changes();

	// on first reset, identify any orphaned streams
	if (m_first_reset)
	{
		m_first_reset = false;

		// put all the streams on the orphan list to start
		for (auto &stream : m_stream_list)
			m_orphan_stream_list[stream.get()] = 0;

		// then walk the graph like we do on update and remove any we touch
		for (speaker_device &speaker : speaker_device_enumerator(machine().root_device()))
		{
			int dummy;
			sound_stream *const output = speaker.output_to_stream_output(0, dummy);
			if (output)
				recursive_remove_stream_from_orphan_list(output);

			m_speakers.emplace_back(speaker);
		}
	}
}


//-------------------------------------------------
//  pause - pause sound output
//-------------------------------------------------

void sound_manager::pause()
{
	mute(true, MUTE_REASON_PAUSE);
}


//-------------------------------------------------
//  resume - resume sound output
//-------------------------------------------------

void sound_manager::resume()
{
	mute(false, MUTE_REASON_PAUSE);
}


//-------------------------------------------------
//  config_load - read and apply data from the
//  configuration file
//-------------------------------------------------

void sound_manager::config_load(config_type cfg_type, config_level cfg_level, util::xml::data_node const *parentnode)
{
	// we only care system-specific configuration
	if ((cfg_type != config_type::SYSTEM) || !parentnode)
		return;

	// iterate over channel nodes
	for (util::xml::data_node const *channelnode = parentnode->get_child("channel"); channelnode != nullptr; channelnode = channelnode->get_next_sibling("channel"))
	{
		mixer_input info;
		if (indexed_mixer_input(channelnode->get_attribute_int("index", -1), info))
		{
			float defvol = channelnode->get_attribute_float("defvol", 1.0f);
			float newvol = channelnode->get_attribute_float("newvol", -1000.0f);
			if (newvol != -1000.0f)
				info.stream->input(info.inputnum).set_user_gain(newvol / defvol);
		}
	}
}


//-------------------------------------------------
//  config_save - save data to the configuration
//  file
//-------------------------------------------------

void sound_manager::config_save(config_type cfg_type, util::xml::data_node *parentnode)
{
	// we only save system-specific configuration
	if (cfg_type != config_type::SYSTEM)
		return;

	// iterate over mixer channels
	for (int mixernum = 0; ; mixernum++)
	{
		mixer_input info;
		if (!indexed_mixer_input(mixernum, info))
			break;

		float const newvol = info.stream->input(info.inputnum).user_gain();
		if (newvol != 1.0f)
		{
			util::xml::data_node *const channelnode = parentnode->add_child("channel", nullptr);
			if (channelnode)
			{
				channelnode->set_attribute_int("index", mixernum);
				channelnode->set_attribute_float("newvol", newvol);
			}
		}
	}
}


//-------------------------------------------------
//  adjust_toward_compressor_scale - adjust the
//  current scale factor toward the current goal,
//  in small increments
//-------------------------------------------------

stream_buffer::sample_t sound_manager::adjust_toward_compressor_scale(stream_buffer::sample_t curscale, stream_buffer::sample_t prevsample, stream_buffer::sample_t rawsample)
{
	stream_buffer::sample_t proposed_scale = curscale;

	// if we want to get larger, increment by 0.01
	if (curscale < m_compressor_scale)
	{
		proposed_scale += 0.01f;
		if (proposed_scale > m_compressor_scale)
			proposed_scale = m_compressor_scale;
	}

	// otherwise, decrement by 0.01
	else
	{
		proposed_scale -= 0.01f;
		if (proposed_scale < m_compressor_scale)
			proposed_scale = m_compressor_scale;
	}

	// compute the sample at the current scale and at the proposed scale
	stream_buffer::sample_t cursample = rawsample * curscale;
	stream_buffer::sample_t proposed_sample = rawsample * proposed_scale;

	// if they trend in the same direction, it's ok to take the step
	if ((cursample < prevsample && proposed_sample < prevsample) || (cursample > prevsample && proposed_sample > prevsample))
		curscale = proposed_scale;

	// return the current scale
	return curscale;
}


//-------------------------------------------------
//  update - mix everything down to its final form
//  and send it to the OSD layer
//-------------------------------------------------

void sound_manager::update(void *ptr, int param)
{
	// determine the duration of this update
	attotime update_period = machine().time() - m_last_update;
	sound_assert(update_period.seconds() == 0);

	// use that to compute the number of samples we need from the speakers
	attoseconds_t sample_rate_attos = HZ_TO_ATTOSECONDS(machine().sample_rate());
	m_samples_this_update = update_period.attoseconds() / sample_rate_attos;

	// recompute the end time to an even sample boundary
	attotime endtime = m_last_update + attotime(0, m_samples_this_update * sample_rate_attos);

	// clear out the mix bufers
	std::fill_n(&m_leftmix[0], m_samples_this_update, 0);
	std::fill_n(&m_rightmix[0], m_samples_this_update, 0);

	// force all the speaker streams to generate the proper number of samples
	for (speaker_device &speaker : m_speakers)
		speaker.mix(&m_leftmix[0], &m_rightmix[0], m_last_update, endtime, m_samples_this_update, (m_muted & MUTE_REASON_SYSTEM));

	// determine the maximum in this section
	stream_buffer::sample_t curmax = 0;
	for (int sampindex = 0; sampindex < m_samples_this_update; sampindex++)
	{
		auto sample = m_leftmix[sampindex];
		if (sample < 0)
			sample = -sample;
		if (sample > curmax)
			curmax = sample;

		sample = m_rightmix[sampindex];
		if (sample < 0)
			sample = -sample;
		if (sample > curmax)
			curmax = sample;
	}

	// pull in current compressor scale factor before modifying
	stream_buffer::sample_t lscale = m_compressor_scale;
	stream_buffer::sample_t rscale = m_compressor_scale;

	// if we're above what the compressor will handle, adjust the compression
	if (curmax * m_compressor_scale > 1.0)
	{
		m_compressor_scale = 1.0 / curmax;
		m_compressor_counter = STREAMS_UPDATE_FREQUENCY / 5;
	}

	// if we're currently scaled, wait a bit to see if we can trend back toward 1.0
	else if (m_compressor_counter != 0)
		m_compressor_counter--;

	// try to migrate toward 0 unless we're going to introduce clipping
	else if (m_compressor_scale < 1.0 && curmax * 1.01 * m_compressor_scale < 1.0)
	{
		m_compressor_scale *= 1.01f;
		if (m_compressor_scale > 1.0)
			m_compressor_scale = 1.0;
	}

	// track whether there are pending scale changes in left/right
	stream_buffer::sample_t lprev = 0, rprev = 0;

	// now downmix the final result
	u32 finalmix_step = machine().video().speed_factor();
	u32 finalmix_offset = 0;
	s16 *finalmix = &m_finalmix[0];
	int sample;
	for (sample = m_finalmix_leftover; sample < m_samples_this_update * 1000; sample += finalmix_step)
	{
		int sampindex = sample / 1000;

		// ensure that changing the compression won't reverse direction to reduce "pops"
		stream_buffer::sample_t lsamp = m_leftmix[sampindex];
		if (lscale != m_compressor_scale && sample != m_finalmix_leftover)
			lscale = adjust_toward_compressor_scale(lscale, lprev, lsamp);

		lprev = lsamp * lscale;
		if (m_compressor_enabled)
			lsamp = lprev;

		// clamp the left side
		if (lsamp > 1.0)
			lsamp = 1.0;
		else if (lsamp < -1.0)
			lsamp = -1.0;
		finalmix[finalmix_offset++] = s16(lsamp * 32767.0);

		// ensure that changing the compression won't reverse direction to reduce "pops"
		stream_buffer::sample_t rsamp = m_rightmix[sampindex];
		if (rscale != m_compressor_scale && sample != m_finalmix_leftover)
			rscale = adjust_toward_compressor_scale(rscale, rprev, rsamp);

		rprev = rsamp * rscale;
		if (m_compressor_enabled)
			rsamp = rprev;

		// clamp the right side
		if (rsamp > 1.0)
			rsamp = 1.0;
		else if (rsamp < -1.0)
			rsamp = -1.0;
		finalmix[finalmix_offset++] = s16(rsamp * 32767.0);
	}
	m_finalmix_leftover = sample - m_samples_this_update * 1000;

	// play the result
	if (finalmix_offset > 0)
	{
		if (!m_nosound_mode)
			machine().osd().update_audio_stream(finalmix, finalmix_offset / 2);
		machine().osd().add_audio_to_recording(finalmix, finalmix_offset / 2);
		machine().video().add_sound_to_recording(finalmix, finalmix_offset / 2);
		if (m_wavfile)
			util::wav_add_data_16(*m_wavfile, finalmix, finalmix_offset);
	}

	// update any orphaned streams so they don't get too far behind
	for (auto &stream : m_orphan_stream_list)
		stream.first->update();

	// remember the update time
	m_last_update = endtime;
	m_update_number++;

	// apply sample rate changes
	apply_sample_rate_changes();

	// notify that new samples have been generated
	emulator_info::sound_hook();
}
