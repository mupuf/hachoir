#include <stdint.h>
#include <iostream>

#include "com_detect.h"
#include "com_decode.h"

#define BURST_MIN_ALLOC_SIZE 100000

#define NOISE_AVR_SAMPLE_COUNT 500
#define NOISE_THRESHOLD_FACTOR 6

#define COMS_DETECT_MIN_SAMPLES 50
#define COMS_DETECT_SAMPLES_UNDER_THRS 10
#define COMS_PREFIX_SAMPLE_COUNT 50

enum rx_state_t {
	LISTEN = 0,
	RX = 1
};

burst_sc16_t
burst_sc16_init()
{
	burst_sc16_t b = {NULL, 0, 0, 0, 0.0, 0, 0.0 };
	return b;
}

inline void
burst_sc16_start(burst_sc16_t *b, float central_freq, size_t bandwidth,
		 float noise_mag_avr, uhd::time_spec_t blk_time, size_t blk_off)
{
	b->len = 0;
	b->central_freq = central_freq;
	b->bandwidth = bandwidth;
	b->noise_mag_avr = noise_mag_avr;
	b->start_time_us = blk_time.to_ticks(1000000); // TODO: Fix the time calculation
}

inline void
burst_sc16_append(burst_sc16_t *b, std::complex<short> *src, size_t src_len)
{
	if (b->allocated_len < b->len + src_len) {
		b->allocated_len = (b->len + src_len) * 2;
		if (b->allocated_len < BURST_MIN_ALLOC_SIZE)
			b->allocated_len = BURST_MIN_ALLOC_SIZE;
	}

	/* make sure we have enough space in the sample buffer */
	b->samples = (std::complex<short> *) realloc(b->samples,
				 b->allocated_len * sizeof(std::complex<short>));
	if (!b->samples) {
		std::cerr << "burst_sc16_append: cannot allocate space for "
			  << b->allocated_len << " samples."
			  << std::endl;
		exit(1);
	}


	/* add the new data at the end of the current transmission */
	memcpy(b->samples + b->len, src, src_len * sizeof(std::complex<short>));
	b->len += src_len;
}


void process_samples_sc16(uhd::rx_metadata_t md, std::complex<short> *samples, size_t count)
{
	/* detection + current state */
	static float noise_avr = -1.0, com_thrs = 200.0, noise_mag_sum;
	static size_t detect_samples_under = 0;
	static rx_state_t state = LISTEN;

	/* communication */
	static std::complex<short> samples_lst_blk[COMS_PREFIX_SAMPLE_COUNT];
	static burst_sc16_t burst = burst_sc16_init();
	static size_t com_sample = 0;
	static float com_mag_sum = 0.0;
	size_t com_blk_start = 0;

	/* for every sample */
	for (size_t i = 0; i < count; i++) {
		short real = samples[i].real();
		short imag = samples[i].imag();
		float mag = sqrtf(real*real + imag*imag);

		/* calculate the noise level and thresholds */
		noise_mag_sum += mag;
		if (i % NOISE_AVR_SAMPLE_COUNT == NOISE_AVR_SAMPLE_COUNT -1) {
			float avr = noise_mag_sum / NOISE_AVR_SAMPLE_COUNT;
			if (noise_avr < 0 || avr < noise_avr) {
				noise_avr = avr;
				com_thrs = noise_avr * NOISE_THRESHOLD_FACTOR;
			}
			noise_mag_sum = 0;
		}

		/* detect the beginning of new transmissions */
		if (mag >= com_thrs) {
			if (state == LISTEN) {
				state = RX;
				burst_sc16_start(&burst, 0.0, 1000000, noise_avr,
						 md.time_spec, i);
				com_mag_sum = 0.0;
				com_sample = 0;
				com_blk_start = i;
			}
			detect_samples_under = 0;
		} else
			detect_samples_under++;

		/* code executed when we are receiving a transmission */
		if (state == RX) {
			com_mag_sum += mag;
			com_sample++;

			/* detect the end of the transmission */
			if (detect_samples_under > COMS_DETECT_SAMPLES_UNDER_THRS) {
				state = LISTEN;

				if (com_sample >= COMS_DETECT_MIN_SAMPLES) {
					size_t com_blk_len = i - com_blk_start;
					burst_sc16_append(&burst, samples + com_blk_start,
							  com_blk_len);
					process_burst_sc16(&burst);

					std::cout << "communication ended, len = "
						  << com_sample
						  << ", avg = "
						  << (com_mag_sum / com_sample) * 100 / com_thrs
						  << "% above threshold ("
						  << com_mag_sum / com_sample
						  << " vs noise avr "
						  << noise_avr
						  << "), start_off = "
						  << com_blk_start
						  << std::endl;
				}
			}
		}
	}

	/* we arrived at the end of this block, but a communication is still
	 * going on. Let's copy the data so as we don't loose it!
	*/
	if (state == RX) {
		burst_sc16_append(&burst, samples + com_blk_start,
				  count - com_blk_start - 1);
	}
}


void process_samples_fc32(uhd::rx_metadata_t md, std::complex<float> *samples, size_t count)
{
	std::cout << "fc32: received " << count << " samples!" << std::endl;
}

void process_samples_fc64(uhd::rx_metadata_t md, std::complex<double> *samples, size_t count)
{
	std::cout << "fc64: received " << count << " samples!" << std::endl;
}

