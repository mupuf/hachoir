#include "fsk.h"

#include "utils/constellation.h"

FSK::FSK()
{
}

/*void freq_get_avr(burst_sc16_t *burst, float &freq_offset, float &freq_std)
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
}*/

#if 0
uint8_t FSK::likeliness(const burst_sc16_t * const burst)
{
	size_t last_crossing = 0;
	Constellation c;

	_cnt_table.reserve(1000);

	// get the average dist
	std::complex<short> *start = &burst->samples[burst->sub_bursts[0].start];
	for (size_t i = 0; i < burst->sub_bursts[0].len - 1; i++) {
		if ((start[i].real() > 0 && start[i + 1].real() <= 0) ||
		    (start[i].real() < 0 && start[i + 1].real() >= 0)) {
			if (last_crossing > 0) {
				size_t cnt = i - last_crossing;
				_cnt_table.push_back(cnt);
				c.addPoint(cnt);
			}
			last_crossing = i;
		}
	}

	c.clusterize();

	//std::cerr << c.histogram();

	/*int i = 0;
	ConstellationPoint cp;
	do {
		cp = c.mostProbabilisticPoint(i);
		if (cp.proba > 0.0)
			std::cout << cp.toString() << " ";
		i++;
	} while (cp.proba > 0.0);
	std::cout << std::endl;*/

	ConstellationPoint cp0 = c.mostProbabilisticPoint(0);
	ConstellationPoint cp1 = c.mostProbabilisticPoint(1);

	// TODO: support more than the 2-FSK (FIXME)
	ConstellationPoint *min = cp0.pos < cp1.pos ? &cp0 : &cp1;
	ConstellationPoint *max = cp0.pos > cp1.pos ? &cp0 : &cp1;
	_threshold = min->pos + (max->pos - min->pos) / 2;
	float freq_diff = (burst->phy.sample_rate / (max->pos - min->pos));

	char phy_params[150];
	snprintf(phy_params, sizeof(phy_params), "2-FSK: [low = %f (p=%.2f), threshold = %u, high = %f (p=%.2f)], freq_diff = %.2f kHz",
		 min->pos, min->proba, _threshold, max->pos, max->proba, freq_diff / 1000);
	_phy_params = phy_params;

	// check the most common frequency values are common-enough
	if (cp0.proba < 0.33 || cp1.proba < 0.10)
		return 0;

	// check the burst doesn't have too many sub bursts
	if (burst->sub_bursts.size() > 10)
		return 0;

	return 255;
}
#endif

#include <fstream>
#define SIG_AVR 5
uint8_t FSK::likeliness(const Burst &burst)
{
	std::vector<float> diff_phase;
	float phase_sum = 0;
	float lastN[SIG_AVR] = { 0.0 };
	size_t lastNpos = 0;
	Constellation cSymb, cTS;
	ConstellationPoint cpTS, cp;

	diff_phase.reserve(10000);

	//FILE *f = fopen("Diff_phase", "w");

	/* compute the phase difference */
	for (size_t i = 0; i < burst.subBursts[0].len - 1; i++) {
		std::complex<float> f1 = std::complex<float>(burst.samples[i].real(), burst.samples[i].imag());
		std::complex<float> f2 = std::complex<float>(burst.samples[i + 1].real(), burst.samples[i + 1].imag());

		float df = fmod((arg(f1) - arg(f2)),2*M_PI);
		if (df < 0)
			df = 2*M_PI + df;
		df = fmod(df+M_PI,2*M_PI);

		lastN[lastNpos] = df;
		lastNpos = (lastNpos + 1) % SIG_AVR;

		float avg = 0.0;
		size_t avg_len = (i + 1) < SIG_AVR ? (i + 1) : SIG_AVR;
		for(size_t i = 0; i < avg_len; i++)
			avg += lastN[i];
		avg /= avg_len;

		//fprintf(f, "%f\n", avg);
		diff_phase.push_back(avg);

		if (i >= SIG_AVR)
			cSymb.addPoint(avg * 1000.0);
		phase_sum += df;
	}

	//fclose(f);

	/* learn the different symbols */
	cSymb.clusterize(0.01, 0.0);

	//std::cerr << cSymb.histogram() << std::endl;

	// get the 0 / 1 symbols and the threshold
	float min = cSymb.histMaxVal(), max = cSymb.histMinVal();
	int i = 0;
	do {
		cp = cSymb.mostProbabilisticPoint(i);
		if (cp.proba > 0.0) {
			if (cp.pos > max && cp.proba > 0.1)
				max = cp.pos;
			if (cp.pos < min && cp.proba > 0.1)
				min = cp.pos;
			std::cout << cp.toString() << " ";
		}
		i++;
	} while (cp.proba > 0.0);
	std::cout << std::endl;

	min /= 1000.0;
	max /= 1000.0;
	_threshold = min + (max - min) / 2.0;

	/* compute the symbol time */
	size_t last_crossing = 0;
	float diff_phase_avr = 0.0;
	for (size_t i = 0; i < diff_phase.size() - 1; i++) {
		if ((diff_phase[i] > _threshold && diff_phase[i + 1] <= _threshold) ||
		    (diff_phase[i] < _threshold && diff_phase[i + 1] >= _threshold)) {
			size_t cnt = i - last_crossing;
			cTS.addPoint(cnt);

			symbolFSK s = { diff_phase_avr / cnt, (float)cnt};
			_cnt_table.push_back(s);
			last_crossing = i;
			diff_phase_avr = 0.0;
		}
		diff_phase_avr += diff_phase[i];
	}

	// add the last point
	size_t cnt = diff_phase.size() - last_crossing;
	cTS.addPoint(cnt);
	diff_phase_avr += diff_phase[diff_phase.size() - 1];
	symbolFSK s = { diff_phase_avr / cnt, (float)cnt};
	_cnt_table.push_back(s);

	/*if (cTS.histValCount() < 30)
		return 0;*/

	cpTS = cTS.findGreatestCommonDivisor();
	_TS = roundf(cpTS.pos);

	char phy_params[150];
	snprintf(phy_params, sizeof(phy_params), "2-FSK: [low = %f (p=%.2f), threshold = %f, high = %f (p=%.2f), symbol time = %f (p=%.2f)]",
		 min, 0.0, _threshold, max, 0.0, _TS, cpTS.proba);
	_phy_params = phy_params;

	return 255;
}

std::vector<Message> FSK::demod(const Burst &burst)
{
	std::vector<Message> msg;
	Message m;

	char filename[100];
	sprintf(filename, "burst_%u_fsk_freq.csv", burst.burstID());
	FILE *f = fopen(filename, "w");

	for (size_t i = 0; i < _cnt_table.size(); i++) {
		bool val = _cnt_table[i].diff_phase < _threshold;
		float bitsCount = roundf(_cnt_table[i].len / _TS);
		//bitsCount = roundf(bitsCount);

		for (size_t i = 0; i < bitsCount; i++)
			m.addBit(val);

		fprintf(f, "%i, %f, %f\n", i, _cnt_table[i].diff_phase, _cnt_table[i].len);
	}
	fclose(f);

	msg.push_back(m);

	return msg;
}

std::string FSK::modulationString() const
{
	return _phy_params;
}
