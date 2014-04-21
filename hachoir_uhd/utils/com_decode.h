#ifndef COM_DECODE_H
#define COM_DECODE_H

#include <stdint.h>
#include <complex>
#include <vector>

#include "phy_parameters.h"

struct sub_burst_sc16_t {
	size_t start;
	size_t end;
	size_t len;

	uint64_t time_start_us;
	uint64_t time_stop_us;
};

struct burst_sc16_t {
	std::complex<short> *samples;
	size_t allocated_len;
	size_t len;

	std::vector<sub_burst_sc16_t> sub_bursts;

	uint64_t burst_id;
	uint64_t start_time_us;
	uint64_t stop_time_us;
	float noise_mag_avr;
	phy_parameters_t phy;
};

void process_burst_sc16(burst_sc16_t *burst);
void freq_get_avr(const burst_sc16_t *burst, float &freq, float &freq_std);

#endif // COM_DECODE_H
