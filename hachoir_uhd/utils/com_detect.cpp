#include <stdint.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "com_detect.h"
#include "com_decode.h"

#define BURST_MIN_ALLOC_SIZE 100000

#define DC_OFFSET_SAMPLE_COUNT 32768
#define NOISE_AVR_SAMPLE_COUNT 2000
#define NOISE_THRESHOLD_FACTOR 3.0

#define COMS_DETECT_MIN_SAMPLES 150
#define COMS_DETECT_SAMPLES_UNDER_THRS 20
#define COMS_DETECT_COALESCING_TIME_US 10000

enum rx_state_t {
	LISTEN = 0,
	COALESCING = 1,
	RX = 2
};

static inline
uint64_t time_from_sample_count(float sample_rate, size_t sample_count)
{
	return sample_count * 1000000 / sample_rate;
}

static inline
uint64_t sample_count_from_time(float sample_rate, uint64_t time_us)
{
	return time_us * sample_rate / 1000000;
}

burst_sc16_t
burst_sc16_init()
{
	burst_sc16_t b = {NULL, 0, 0, std::vector<sub_burst_sc16_t>(), 0, 0, 0, 0.0 };
	return b;
}

inline void
burst_sc16_sub_start(burst_sc16_t *b)
{
	struct sub_burst_sc16_t sb;

	sb.start = b->len;
	sb.end = 0;
	sb.time_start_us = b->start_time_us;
	sb.time_start_us += time_from_sample_count(b->phy.sample_rate, b->len);
	sb.time_stop_us = 0;
	sb.len = 0;

	/*uint64_t prev = 0;
	if (b->sub_bursts.size() > 0)
		prev = b->sub_bursts.back().time_stop_us;

	fprintf(stderr, "	substart[%i]: time = %u µs, start = %u\n",
		b->sub_bursts.size(), sb.time_start_us, sb.start);*/

	b->sub_bursts.push_back(sb);
}

inline void
burst_sc16_sub_cancel_current(burst_sc16_t *b)
{
	if (b->sub_bursts.back().end == 0) {
		b->sub_bursts.pop_back();
		//fprintf(stderr, "	subcancel\n");
	}
	else {
		std::cerr << "burst_sc16_sub_cancel_current: "
			  << "Tried to cancel a finished sub-burst"
			  << std::endl;
	}
}

inline void
burst_sc16_sub_stop(burst_sc16_t *b)
{
	struct sub_burst_sc16_t &sb = b->sub_bursts.back();

	sb.end = b->len - COMS_DETECT_SAMPLES_UNDER_THRS;
	sb.time_stop_us = b->start_time_us;
	sb.time_stop_us += time_from_sample_count(b->phy.sample_rate, b->len);

	sb.len = sb.end - sb.start;

	if (sb.len == 0 || sb.end < sb.start)
		burst_sc16_sub_cancel_current(b);

	/*fprintf(stderr, "	subend: len = %u (time = %u µs)\n", sb.len,
		sb.time_stop_us - sb.time_start_us);*/
}

inline void
burst_sc16_start(burst_sc16_t *b, const phy_parameters_t &phy,
		 float noise_mag_avr, uint64_t time_us, size_t blk_off)
{
	b->len = 0;
	b->sub_bursts.clear();
	b->phy = phy;
	b->noise_mag_avr = noise_mag_avr;
	b->burst_id = -1;
	b->start_time_us = time_us;
	b->start_time_us += time_from_sample_count(phy.sample_rate, blk_off);
	b->stop_time_us = 0;

	b->sub_bursts.clear();

	/*fprintf(stderr, "start: pos = %u\n", blk_off);*/
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

	/*fprintf(stderr, "end: pos = %u\n", b->len);*/

	/*fprintf(stderr, "b->sub_bursts.back().end = %p\n", b->sub_bursts.back().end);
	fprintf(stderr, "b->sub_bursts.front().start = %p\n", b->sub_bursts.front().start);*/

	if (b->sub_bursts.back().end < b->sub_bursts.front().start)
		b->len = 0;
	else
		b->len = b->sub_bursts.back().end - b->sub_bursts.front().start;

	b->burst_id = burst_id++;
	b->stop_time_us = b->start_time_us;
	b->stop_time_us += time_from_sample_count(b->phy.sample_rate, b->len);
}

inline void
process_sc16_dump_samples(FILE* file, std::complex<short> IQ, float mag,
			  float noise_avr, bool com_detected)
{
	/*fprintf(file, "%hi, %hi, %f, %f, %u\n",
	       IQ.real(), IQ.imag(), mag, noise_avr, com_detected?127:0);*/
}

process_return_val_t
process_samples_sc16(phy_parameters_t &phy, uint64_t time_us,
				      std::complex<short> *samples, size_t count)
{
	/* output */
	//static FILE* file = fopen("com_detect_output.csv", "w");

	/* detection + current state */
	static float noise_mag_max = -1.0, com_thrs = 100000.0, noise_cur_max = 0.0;
	static float I_avr = 0.0, Q_avr = 0.0, I_sum = 0.0, Q_sum = 0.0;
	static size_t IQ_count = 0.0, detect_samples_under = 0;
	static rx_state_t state = LISTEN;

	/* communication */
	static burst_sc16_t burst = burst_sc16_init();
	static size_t com_sample = 0, com_sub_sample = 0;
	static float com_mag_sum = 0.0;
	size_t com_blk_start = 0;
	size_t com_coalescing_samples_count = sample_count_from_time(phy.sample_rate,
								     COMS_DETECT_COALESCING_TIME_US);
	/* for every sample */
	for (size_t i = 0; i < count; i++) {
		float I = samples[i].real() - I_avr;
		float Q = samples[i].imag() - Q_avr;
		float mag = sqrtf(I*I + Q*Q);

		/* calculate the I/Q DC-offsets */
		I_sum += samples[i].real();
		Q_sum += samples[i].imag();
		if (IQ_count++ % DC_OFFSET_SAMPLE_COUNT == DC_OFFSET_SAMPLE_COUNT - 1) {
			I_avr = I_sum / IQ_count;
			Q_sum = Q_sum / IQ_count;
			IQ_count = 0;
			I_sum = Q_sum = 0.0;
		}

		//process_sc16_dump_samples(file, samples[i], mag, noise_avr, state == RX);

		/* calculate the noise level and thresholds */
		if (mag > noise_cur_max)
			noise_cur_max = mag;
		if (i % NOISE_AVR_SAMPLE_COUNT == NOISE_AVR_SAMPLE_COUNT -1) {
			if (noise_mag_max < 0 || noise_cur_max < noise_mag_max) {
				noise_mag_max = noise_cur_max;
				com_thrs = noise_mag_max * NOISE_THRESHOLD_FACTOR;
			}
			//fprintf(stderr, "noise_mag_max = %f, noise_cur_max = %f\n", noise_mag_max, noise_cur_max);
			noise_cur_max = 0.0;
		}

		/* detect the beginning of new transmissions */
		if (mag >= com_thrs) {
			if (state == LISTEN) {
				state = RX;
				burst_sc16_start(&burst, phy, noise_mag_max, time_us, i);
				burst_sc16_sub_start(&burst);
				com_mag_sum = 0.0;
				com_sample = 0;
				com_sub_sample = 0;
				com_blk_start = i;
			} else if(state == COALESCING) {
				size_t com_blk_len = i - com_blk_start;
				burst_sc16_append(&burst, samples + com_blk_start,
								  com_blk_len);
				burst_sc16_sub_start(&burst);
				com_sub_sample = 0;

				state = RX;
				com_blk_start = i;
			}
			detect_samples_under = 0;
		} else
			detect_samples_under++;

		/* code executed when we are receiving a transmission */
		if (state > LISTEN) {
			com_sample++;

			if (state == RX) {
				com_mag_sum += mag;
				com_sub_sample++;

				/* detect the end of the transmission */
				if (detect_samples_under >= COMS_DETECT_SAMPLES_UNDER_THRS) {
					size_t com_blk_len = i - com_blk_start;
					burst_sc16_append(&burst, samples + com_blk_start,
							  com_blk_len);
					com_blk_start = i;

					if (com_sub_sample >= COMS_DETECT_MIN_SAMPLES) {
						burst_sc16_sub_stop(&burst);
						detect_samples_under = 0;
					} else {
						burst_sc16_sub_cancel_current(&burst);
					}
					state = COALESCING;
				}
			} else if (state == COALESCING &&
				   detect_samples_under >= com_coalescing_samples_count) {
				state = LISTEN;

				if (com_sample >= COMS_DETECT_MIN_SAMPLES) {
					if (burst.sub_bursts.size() > 0) {

						burst_sc16_done(&burst);

						std::cerr << burst.burst_id
							  << ": new communication, time = "
							  << burst.start_time_us
							  << " µs, len = "
							  << burst.len
							  << " samples, len_time = "
							  << burst.stop_time_us - burst.start_time_us
							  << " µs, sub-burst = "
							  << burst.sub_bursts.size()
							  << ", avg_pwr = "
							  << (com_mag_sum / com_sample) * 100 / com_thrs
							  << "% above threshold ("
							  << com_mag_sum / com_sample
							  << " vs noise mag max "
							  << noise_mag_max
							  << ")"
							  << std::endl;

						process_burst_sc16(&burst);

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

	}

	/* we arrived at the end of this block, but a communication is still
	 * going on. Let's copy the data so as we don't loose it!
	*/
	if (state > LISTEN) {
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

