#include "burst.h"

#include <iostream>
#include <string.h>

#define BURST_MIN_ALLOC_SIZE 100000

static inline
uint64_t time_from_sample_count(float sample_rate, size_t sample_count)
{
	return sample_count * 1000000 / sample_rate;
}

bool Burst::bumpBufferSize(size_t len)
{
	if (_allocated_len < len) {
		_allocated_len = len * 2;
		if (_allocated_len < BURST_MIN_ALLOC_SIZE)
			_allocated_len = BURST_MIN_ALLOC_SIZE;
	}

	/* make sure we have enough space in the sample buffer */
	samples = (std::complex<short> *) realloc(samples,
				 _allocated_len * sizeof(std::complex<short>));
	if (!samples) {
		std::cerr << "Burst: cannot allocate space for "
			  << _allocated_len << " samples."
			  << std::endl;
		return false;
	}

	return true;
}

Burst::Burst() : _allocated_len(0), _len(0), _burst_id(0), _start_time_us(0),
		 _stop_time_us(0), _noise_mag_avr(0), samples(NULL)
{
	bumpBufferSize(1);
}

void Burst::start(const phy_parameters_t &phy, float noise_mag_avr,
		  uint64_t time_us, size_t blk_off)
{
	_len = 0;
	subBursts.clear();
	_phy = phy;
	_noise_mag_avr = noise_mag_avr;
	_burst_id = -1;
	_start_time_us = time_us;
	_start_time_us += time_from_sample_count(phy.sample_rate, blk_off);
	_stop_time_us = 0;

	subStart();
}

bool Burst::append(std::complex<short> *src, size_t src_len)
{
	if (!bumpBufferSize(_len + src_len))
		return false;

	/* add the new data at the end of the current transmission */
	memcpy(samples + _len, src, src_len * sizeof(std::complex<short>));
	_len += src_len;

	return true;
}

void Burst::done()
{
	static uint64_t burst_id = 0;

	if (subBursts.back().end < subBursts.front().start)
		_len = 0;
	else
		_len = subBursts.back().end - subBursts.front().start;

	_burst_id = burst_id++;
	_stop_time_us = _start_time_us;
	_stop_time_us += time_from_sample_count(_phy.sample_rate, _len);
}

void Burst::subStart()
{
	struct sub_burst_t sb;

	sb.start = _len;
	sb.end = 0;
	sb.time_start_us = _start_time_us;
	sb.time_start_us += time_from_sample_count(_phy.sample_rate, _len);
	sb.time_stop_us = 0;
	sb.len = 0;

	/*uint64_t prev = 0;
	if (_sub_bursts.size() > 0)
		prev = _sub_bursts.back().time_stop_us;

	fprintf(stderr, "	substart[%i]: time = %u µs, start = %u\n",
		_sub_bursts.size(), sb.time_start_us, sb.start);*/

	subBursts.push_back(sb);
}

void Burst::subCancel()
{
	if (subBursts.back().end == 0) {
		subBursts.pop_back();
		//fprintf(stderr, "	subcancel\n");
	}
	else {
		std::cerr << "burst_sub_cancel_current: "
			  << "Tried to cancel a finished sub-burst"
			  << std::endl;
	}
}

void Burst::subStop(size_t trimSampleCount)
{
	struct sub_burst_t &sb = subBursts.back();

	sb.end = _len - trimSampleCount;
	sb.time_stop_us = _start_time_us;
	sb.time_stop_us += time_from_sample_count(_phy.sample_rate, _len);

	sb.len = sb.end - sb.start;

	if (sb.len == 0 || sb.end < sb.start)
		subCancel();
}
