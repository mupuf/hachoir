#include "sig_proc.h"

#include <iostream>


void freq_get_avr(const Burst &burst, float &freq, float &freq_std)
{
	// get the frequency of the signal
	uint64_t sum_cnt = 0, sum_cnt_sq = 0, count_cnt = 0;
	size_t last_crossing = 0, samples_count = 0;
	float diff_phase_sum = 0.0;

	for (size_t b = 0; b < burst.subBursts.size(); b++) {
		const std::complex<short> *start = &burst.samples[burst.subBursts[b].start];

		last_crossing = 0;
		for (size_t i = 0; i < burst.subBursts[b].len - 1; i++) {
			if ((start[i].real() > 0 && start[i + 1].real() <= 0) ||
			    (start[i].real() < 0 && start[i + 1].real() >= 0)) {
				if (last_crossing > 0) {
					size_t len = (i - last_crossing);
					sum_cnt += len;
					sum_cnt_sq += len * len;
					count_cnt++;
				}
				last_crossing = i;
			}

			std::complex<float> f1 = std::complex<float>(burst.samples[i].real(), burst.samples[i].imag());
			std::complex<float> f2 = std::complex<float>(burst.samples[i + 1].real(), burst.samples[i + 1].imag());

			float df = fmod((arg(f1) - arg(f2)),2*M_PI);
			if (df < 0)
				df = 2*M_PI + df;
			df = fmod(df+M_PI,2*M_PI);
			diff_phase_sum += arg(f1) - arg(f2); //df;
			samples_count++;
		}
	}

	float sample_avr = sum_cnt * 1.0 / count_cnt;
	float sample_avr_sq = sum_cnt_sq * 1.0 / count_cnt;
	float freq_offset = (burst.phy().sample_rate / sample_avr) / 2;
	if (diff_phase_sum < 0)
		freq_offset *= -1.0;
	//float diff_phase_avr = diff_phase_sum / samples_count;
	//float freq_offset = (diff_phase_avr * burst->phy.sample_rate) / (2 * M_PI);
	freq = (burst.phy().central_freq + freq_offset);
	freq_std = sqrtf(sample_avr_sq - (sample_avr * sample_avr));
}
