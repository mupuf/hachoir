#include "com_decode.h"
#include <stdio.h>
#include <string.h>

#include <iostream>

#include "ook.h"

void process_burst_sc16(burst_sc16_t *burst)
{
	OOK ook;
	Demodulator *demod[] = {
		&ook,
		// Add demodulations here
	};

	Demodulator *fittest = NULL;
	uint8_t bestScore = 0;

	for (int i = 0; i < sizeof(demod) / sizeof(Demodulator *); i++) {
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

	Message m = fittest->demod(burst);

	std::cerr << "New decoded message: " << m.toString(Message::BINARY) << std::endl << std::endl;

	/*char filename[100];
	sprintf(filename, "burst_%i.csv", burst->burst_id);
	FILE *f = fopen(filename, "wb");
	for (int i = 0; i < burst->len; i++) {
		fprintf(f, "%i, %i\n", burst->samples[i].real(), burst->samples[i].imag());
	}
	fclose(f);*/

	/*char filename[100];
	sprintf(filename, "burst_%i.dat", burst->burst_id);
	FILE *f = fopen(filename, "wb");
	fwrite(burst->samples, sizeof(std::complex<short>), burst->len, f);
	fclose(f);*/


}
