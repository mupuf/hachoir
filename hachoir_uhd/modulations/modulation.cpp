#include "modulation.h"

Modulation::Modulation(float centralFreq) : _freq(centralFreq)
{
}


void Modulation::modulate(std::complex<short> *samples, size_t len)
{
	float radian_step = 2 * M_PI * (_freq - _carrier_freq) / _sample_rate;
	short I, Q;

	//FILE *f = fopen("samples.csv", "a");
	for (size_t i = 0; i < len; i++) {
		I = _amp * cosf(_phase);
		Q = _amp * sinf(_phase);
		_phase += radian_step;
		samples[i] = std::complex<short>(I, Q);
		//fprintf(f, "%i, %i\n", I, Q);
	}
	//fclose(f);
}
