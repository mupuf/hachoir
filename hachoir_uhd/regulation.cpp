#include "regulation.h"

Regulation regDB;

RegulationBand::RegulationBand()
{
}

RegulationBand::RegulationBand(float freqStart, float freqEnd, size_t channelCount,
	       int modulationMask) :
	_freqStart(freqStart), _freqEnd(freqEnd), _channelCount(channelCount),
	_modulationMask(modulationMask)
{
}

float RegulationBand::channelWidth() const
{
	return (_freqEnd - _freqStart) / _channelCount;
}

#include <stdio.h>
bool RegulationBand::frequencyIsInBand(float freq, size_t *channel)
{
	fprintf(stderr, "freq = %f, _freqStart = %f, _freqEnd = %f\n", freq, _freqStart, _freqEnd);
	if (freq >= _freqStart && freq <= _freqEnd) {
		if (channel)
			*channel = (freq - _freqStart) / channelWidth();
		return true;
	} else
		return false;
}

Regulation::Regulation()
{
	RegulationBand ISM_433(433.075e6, 434.775e6, 69, Modulation::OOK | Modulation::FSK);

	_bands.push_back(ISM_433);
}

bool Regulation::bandAtFrequencyRange(float freqStart, float freqEnd, RegulationBand &band)
{
	for (size_t i = 0; i < _bands.size(); i++) {
		if (_bands[i].frequencyIsInBand(freqStart) &&
		     _bands[i].frequencyIsInBand(freqEnd)) {
			band = _bands[i];
			return true;
		}
	}

	return false;
}
