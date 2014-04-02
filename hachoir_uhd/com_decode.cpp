#include "com_decode.h"
#include <stdio.h>
#include <string.h>

#include <iostream>

#include "ook.h"
#include "fsk.h"
#include "manchester.h"

static void burst_dump_samples(burst_sc16_t *burst)
{
	char filename[100];
	sprintf(filename, "burst_%i.dat", burst->burst_id);
	FILE *f = fopen(filename, "wb");
	fwrite(burst->samples, sizeof(std::complex<short>), burst->len, f);
	fclose(f);

	sprintf(filename, "burst_%i.csv", burst->burst_id);
	f = fopen(filename, "wb");
	size_t b = 0;
	for (size_t i = 0; i < burst->len; i++) {
		size_t inSubBurst = 0;
		if (i >= burst->sub_bursts[b].start) {
			inSubBurst = (i < burst->sub_bursts[b].end) ? 127 : 0;
			if (i > burst->sub_bursts[b].end && b < burst->sub_bursts.size())
				b++;
		}

		fprintf(f, "%i, %i, %i\n", burst->samples[i].real(), burst->samples[i].imag(), inSubBurst);
	}
	fclose(f);
}

void freq_get_avr(const burst_sc16_t *burst, float &freq, float &freq_std)
{
	// get the frequency of the signal
	uint64_t sum_cnt = 0, sum_cnt_sq = 0, count_cnt = 0;
	size_t last_crossing = 0;
	float diff_phase_sum = 0.0;

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

			std::complex<float> f1 = std::complex<float>(burst->samples[i].real(), burst->samples[i].imag());
			std::complex<float> f2 = std::complex<float>(burst->samples[i + 1].real(), burst->samples[i + 1].imag());

			float df = fmod((arg(f1) - arg(f2)),2*M_PI);
			if (df < 0)
				df = 2*M_PI + df;
			df = fmod(df+M_PI,2*M_PI);
			diff_phase_sum += df;
		}
	}

	float sample_avr = sum_cnt * 1.0 / count_cnt;
	float sample_avr_sq = sum_cnt_sq * 1.0 / count_cnt;
	float freq_offset = (burst->phy.sample_rate / sample_avr) / 2;
	if (diff_phase_sum < 0)
		freq_offset *= -1.0;
	freq = (burst->phy.central_freq + freq_offset);
	freq_std = sqrtf(sample_avr_sq - (sample_avr * sample_avr));
}

void process_burst_sc16(burst_sc16_t *burst)
{
	// List of available demodulators
	OOK ook;
	FSK fsk;
	Demodulator *demod[] = {
		&ook,
		&fsk,
		// Add demodulations here
	};

	// dump the samples to files
	burst_dump_samples(burst);

	Demodulator *fittest = NULL;
	uint8_t bestScore = 0;

	for (size_t i = 0; i < sizeof(demod) / sizeof(Demodulator *); i++) {
		Demodulator *d = demod[i];
		uint8_t score = d->likeliness(burst);
		if (!fittest || score > bestScore) {
			bestScore = score;
			fittest = d;
		}
	}

	// bail out if the score is very low!
	if (bestScore < 128) {
		std::cerr << "Demod: burst ID "
			  << burst->burst_id
			  << " has an unknown modulation. Len = "
			  << burst->stop_time_us - burst->start_time_us
			  << " µs, sub-burst count = "
			  << burst->sub_bursts.size()
			  << std::endl << std::endl;

		return;
	}

	std::vector<Message> msgs = fittest->demod(burst);
	std::cerr << "New message: modulation = '" << msgs[0].modulation()
		  << "', sub messages = " << msgs.size() << std::endl;
	for (size_t i = 0; i < msgs.size(); i++) {
		std::cerr << "Sub msg " << i << ": len = " << msgs[i].size() << ": " << std::endl
		<< "BIN: " << msgs[i].toString(Message::BINARY) << std::endl
		<< "HEX: " << msgs[i].toString(Message::HEX) << std::endl;

		Message man;
		if (Manchester::decode(msgs[i], man)) {
			std::cerr << "Manchester code detected:" << std::endl
				  << "BIN: " << man.toString(Message::BINARY) << std::endl
				  << "HEX: " << man.toString(Message::HEX) << std::endl;
		}

		std::cerr << std::endl;
	}
}
