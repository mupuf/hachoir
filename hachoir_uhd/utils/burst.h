#ifndef BURST_H
#define BURST_H

#include <vector>
#include <complex>

#include <stddef.h>

#include "utils/phy_parameters.h"

class Burst
{
public:
	struct sub_burst_t {
		size_t start;
		size_t end;
		size_t len;

		uint64_t time_start_us;
		uint64_t time_stop_us;
	};

private:
	std::complex<short> *_samples;
	size_t _allocated_len;
	size_t _len;

	std::vector<sub_burst_t> _sub_bursts;

	uint64_t _burst_id;
	uint64_t _start_time_us;
	uint64_t _stop_time_us;
	float _noise_mag_avr;
	phy_parameters_t _phy;

	bool bumpBufferSize(size_t len);
public:
	Burst();

	uint64_t burstID() const { return _burst_id; }
	size_t len() const { return _len; }
	size_t lenTimeUs() const { return _stop_time_us - _start_time_us; }
	uint64_t startTimeUs() const { return _start_time_us; }
	uint64_t stopTimeUs() const { return _stop_time_us; }
	float noiseMagAvr() const { return _noise_mag_avr; }
	phy_parameters_t phy() const { return _phy; }
	const std::complex<short> *samples() const { return _samples; }
	std::vector<sub_burst_t> subBursts() const { return _sub_bursts; }

	void start(const phy_parameters_t &phy, float noise_mag_avr,
		   uint64_t time_us, size_t blk_off);
	bool append(std::complex<short> *src, size_t src_len);
	void done();

	void subStart();
	void subCancel();
	void subStop(size_t trimSampleCount);
};

#endif // BURST_H
