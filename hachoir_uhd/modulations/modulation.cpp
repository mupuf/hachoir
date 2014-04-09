#include "modulation.h"

Modulation::Modulation(float centralFreq) : _freq(centralFreq)
{
}

float Modulation::centralFrequency() const
{
	return _freq;
}
