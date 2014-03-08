#include <stdint.h>
#include <iostream>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

#include "com_detect.h"
#include "com_decode.h"

#define BURST_MIN_ALLOC_SIZE 100000

#define NOISE_AVR_SAMPLE_COUNT 1000
#define NOISE_THRESHOLD_FACTOR 10

#define COMS_DETECT_MIN_SAMPLES 200
#define COMS_DETECT_SAMPLES_UNDER_THRS 20

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
burst_sc16_start(burst_sc16_t *b, const phy_parameters_t &phy,
		 float noise_mag_avr, uint64_t time_us, size_t blk_off)
{
	size_t sec_ticks = 1000000;

	b->len = 0;
	b->phy = phy;
	b->noise_mag_avr = noise_mag_avr;
	b->burst_id = -1;
	b->start_time_us = time_us;
	b->start_time_us += (blk_off * sec_ticks) / sec_ticks;
	b->stop_time_us = 0;
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

inline void
burst_sc16_done(burst_sc16_t *b)
{
	static uint64_t burst_id = 0;
	size_t sec_ticks = 1000000;

	b->burst_id = burst_id++;
	b->stop_time_us = b->start_time_us + (b->len * sec_ticks) / sec_ticks;
}

process_return_val_t
process_samples_sc16(phy_parameters_t &phy, uint64_t time_us,
				      std::complex<short> *samples, size_t count)
{
	/* detection + current state */
	static float noise_avr = -1.0, com_thrs = 200.0, noise_mag_sum;
	static size_t detect_samples_under = 0;
	static rx_state_t state = LISTEN;

	/* communication */
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
				burst_sc16_start(&burst, phy, noise_avr, time_us, i);
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
			if (detect_samples_under >= COMS_DETECT_SAMPLES_UNDER_THRS) {
				state = LISTEN;

				if (com_sample >= COMS_DETECT_MIN_SAMPLES) {
					size_t com_blk_len = i - com_blk_start;
					burst_sc16_append(&burst, samples + com_blk_start,
							  com_blk_len);
					burst_sc16_done(&burst);
					process_burst_sc16(&burst);

					std::cout << burst.burst_id
						  << ": new communication, time = "
						  << burst.start_time_us
						  << " µs, len = "
						  << com_sample
						  << " samples, avg_pwr = "
						  << (com_mag_sum / com_sample) * 100 / com_thrs
						  << "% above threshold ("
						  << com_mag_sum / com_sample
						  << " vs noise avr "
						  << noise_avr
						  << ")"
						  << std::endl;

					/* change the phy parameters, if wanted */
					/*phy.central_freq = 869.5e6;
					phy.sample_rate = 1500000;
					phy.IF_bw = 2000000;
					phy.gain = 45;
					return RET_CH_PHY;
					*/
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

	return RET_NOP;
}


process_return_val_t process_samples_fc32(phy_parameters_t &phy, uint64_t time_us, std::complex<float> *samples, size_t count)
{
	std::cout << "fc32: received " << count << " samples!" << std::endl;
	return RET_NOP;
}

process_return_val_t process_samples_fc64(phy_parameters_t &phy, uint64_t time_us, std::complex<double> *samples, size_t count)
{
	std::cout << "fc64: received " << count << " samples!" << std::endl;
	return RET_NOP;
}

