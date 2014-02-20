#include "com_detect.h"
#include <stdint.h>
#include <iostream>

#define NOISE_AVR_SAMPLE_COUNT 500
#define NOISE_THRESHOLD_FACTOR 6.0
#define COMS_SAMPLES_UNDER_THRS 10

enum rx_state_t {
	LISTEN = 0,
	RX = 1
};

void process_samples_sc16(uhd::rx_metadata_t md, std::complex<short> *samples, size_t count)
{
	static float noise_avr = -1.0, com_thrs = 200.0;
	static uint64_t samples_under = 0, coms_sample;
	static rx_state_t state = LISTEN;
	double sum_tot = 0.0, sum_sec = 0.0, sum_com = 0.0;

	/* for every sample */
	for (int i = 0; i < count; i++) {
		float real = samples[i].real();
		float imag = samples[i].imag();
		float mag = sqrtf(real*real + imag*imag);

		/* calculate the noise level and thresholds */
		sum_sec += mag;
		sum_tot += mag;
		if (i % NOISE_AVR_SAMPLE_COUNT == NOISE_AVR_SAMPLE_COUNT -1) {
			float avr = sum_sec / NOISE_AVR_SAMPLE_COUNT;
			if (noise_avr < 0 || avr < noise_avr) {
				noise_avr = avr;
				com_thrs = noise_avr * NOISE_THRESHOLD_FACTOR;
			}
			sum_sec = 0;
		}

		/* detect the beginning of new transmissions */
		if (state == LISTEN && mag >= com_thrs) {
			state = RX;
			samples_under = 0;
			sum_com = 0.0;
			coms_sample = 0;

		} else
			samples_under++;

		/* code executed when we are receiving a transmission */
		if (state == RX) {
			sum_com += mag;
			coms_sample++;

			/* detect the end of the transmission */
			if (samples_under > COMS_SAMPLES_UNDER_THRS) {
				state = LISTEN;
				std::cout << "communication ended, len = " << coms_sample << ", avg = " << sum_com * 100.0 / coms_sample << "% above threshold!" << std::endl;
			}
		}
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

