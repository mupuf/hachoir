#ifndef COM_DECODE_H
#define COM_DECODE_H

#include <stdint.h>
#include <complex>

struct burst_sc16_t {
	std::complex<short> *samples;
	size_t allocated_len;
	size_t len;

	uint64_t start_time_us;
	float central_freq;
	size_t bandwidth;
	float noise_mag_avr;
};

void process_burst_sc16(burst_sc16_t *burst);

#endif // COM_DECODE_H
