#ifndef MODULATION_H
#define MODULATION_H

#include <string>
#include <vector>
#include <complex>

class Modulation
{
private:
	float _freq;

public:
	enum ModulationType {
		UNKNOWN	= 0,
		OOK	= 1,
		FSK	= 2,
		PSK	= 4,
		DPSK	= 8,
		QAM	= 16
	};

	Modulation(float centralFreq);

	float centralFrequency() const;

	virtual std::string toString() const = 0;

	virtual bool genSamples(std::complex<short> **samples, size_t *len, float carrier_freq, float sample_rate) = 0;
};

#endif // MODULATION_H
