#include "modulation.h"

Modulation::Modulation(float centralFreq) : _freq(centralFreq)
{
}


void Modulation::modulate(std::complex<short> *samples, size_t len)
{
	float radian_step = 2 * M_PI * (_carrier_freq - _freq) / _sample_rate;
	short I, Q;

	FILE *f = fopen("samples.csv", "a");
	for (size_t i = 0; i < len; i++) {
		if (_amp != 0.0) {
			float sin, cos;
			sincosf(_phase, &cos, &sin);
			I = _amp * cos;
			Q = _amp * sin;
		} else {
			I = 0;
			Q = 0;
		}
		_phase += radian_step;
		samples[i] = std::complex<short>(I, Q);
		fprintf(f, "%i, %i\n", I, Q);
	}
	fclose(f);
}
