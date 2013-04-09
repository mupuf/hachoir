#include "powerspectrum.h"

PowerSpectrum::PowerSpectrum(qreal freqStart, qreal freqEnd, uint64_t timeNs,
			     uint16_t steps, const char *data) :
	_freqStart(freqStart), _freqEnd(freqEnd),
	_pwrMin(125), _pwrMax(-125), _timeNs(timeNs), _steps(steps)
{
	_pwr.reset(new char[steps]);

	memcpy(_pwr.data(), data, steps);
	for (uint16_t i = 0; i < steps; i++)
	{
		char pwr = data[i];

		if (pwr < _pwrMin)
			_pwrMin = pwr;
		if (pwr > _pwrMax)
			_pwrMax = pwr;
	}
}

void PowerSpectrum::reset(qreal freqStart, qreal freqEnd, uint64_t timeNs,
			     uint16_t steps, const char *data)
{
	_freqStart = freqStart;
	_freqEnd = freqEnd;
	_timeNs = timeNs;

	_pwrMin = 125;
	_pwrMax = -125;

	memcpy(_pwr.data(), data, steps);
	for (uint16_t i = 0; i < steps; i++)
	{
		char pwr = data[i];

		if (pwr < _pwrMin)
			_pwrMin = pwr;
		if (pwr > _pwrMax)
			_pwrMax = pwr;
	}
}
