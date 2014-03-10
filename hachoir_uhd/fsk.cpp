#include "fsk.h"

FSK::FSK()
{
}

void freq_get_avr(burst_sc16_t *burst, float &freq_offset, float &freq_std)
{
	// get the frequency of the signal
	uint64_t sum_cnt = 0, sum_cnt_sq = 0, count_cnt = 0;
	size_t last_crossing = 0;

	for (size_t b = 0; b < burst->sub_bursts.size(); b++) {
		std::complex<short> *start = &burst->samples[burst->sub_bursts[b].start];

		last_crossing = 0;
		for (size_t i = 0; i < burst->sub_bursts[b].len - 1; i++) {
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
		}
	}

	// store the PHY parameters in a single string
	float sample_avr = sum_cnt * 1.0 / count_cnt;
	float sample_avr_sq = sum_cnt_sq * 1.0 / count_cnt;
	freq_offset = (burst->phy.sample_rate / sample_avr) / 2;
	freq_std = sqrtf(sample_avr_sq - (sample_avr * sample_avr));
	fprintf(stderr, "crap: freq = %.03f MHz or %.03f MHz (std = %f%%)\n",
		(burst->phy.central_freq + freq_offset) / 1000000.0,
		(burst->phy.central_freq - freq_offset) / 1000000.0,
		freq_std * 100.0 / freq_offset);
}

uint8_t FSK::likeliness(const burst_sc16_t * const burst)
{
	uint64_t sum_cnt = 0;
	size_t cnt_min = ~0, cnt_max = 0, last_crossing = 0;
	size_t const_low, const_high;

	_cnt_table.reserve(1000);

	// get the average dist
	std::complex<short> *start = &burst->samples[burst->sub_bursts[0].start];
	for (size_t i = 0; i < burst->sub_bursts[0].len - 1; i++) {
		if ((start[i].real() > 0 && start[i + 1].real() <= 0) ||
		    (start[i].real() < 0 && start[i + 1].real() >= 0)) {
			if (last_crossing > 0) {
				size_t cnt = i - last_crossing;
				sum_cnt += cnt;
				if (cnt > cnt_max) cnt_max = cnt;
				if (cnt < cnt_min) cnt_min = cnt;
				_cnt_table.push_back(cnt);
			}
			last_crossing = i;
		}
	}

	// TODO: Create constellations and try to map the samples to it!
	if (cnt_max - cnt_min < 10)
		return 0;

	// TODO: support more than the 2-FSK (FIXME)
	_threshold = sum_cnt * 1.0 / _cnt_table.size();
	const_low = cnt_min;
	const_high = cnt_max;

	//
	float freq_diff = (burst->phy.sample_rate / (const_high - const_low));

	char phy_params[150];
	snprintf(phy_params, sizeof(phy_params), "2-FSK: [%u, %u, %u], freq_diff = %u Hz",
		 cnt_min, _threshold, cnt_max, freq_diff);
	_phy_params = phy_params;

	return 255;
}

Message FSK::demod(const burst_sc16_t * const burst)
{
	Message m(_phy_params);

	char filename[100];
	sprintf(filename, "burst_%u_fsk_freq.csv", burst->burst_id);
	FILE *f = fopen(filename, "w");

	for (size_t i = 0; i < _cnt_table.size(); i++) {
		if (_cnt_table[i] > _threshold)
			m.addBit(true);
		else
			m.addBit(false);
		fprintf(f, "%i, %i\n", i, _cnt_table[i]);
	}
	fclose(f);

	return m;
}
