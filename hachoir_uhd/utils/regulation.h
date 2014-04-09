#ifndef REGULATION_H
#define REGULATION_H

#include "modulations/modulation.h"
#include <vector>

class RegulationBand
{
	float _freqStart;
	float _freqEnd;
	size_t _channelCount;
	int _modulationMask;

public:
	RegulationBand();
	RegulationBand(float freqStart, float freqEnd, size_t channelCount,
		       int modulationMask);

	float channelWidth() const;
	bool frequencyIsInBand(float freq, size_t *channel = NULL);
};

class Regulation
{
	std::vector<RegulationBand> _bands;
public:
	Regulation();

	bool bandAtFrequencyRange(float freqStart, float freqEnd, RegulationBand &band);
};

extern Regulation regDB;

#endif // REGULATION_H
