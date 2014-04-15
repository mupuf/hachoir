#ifndef MODULATION_H
#define MODULATION_H

#include <string>
#include <vector>
#include <complex>

#include "utils/message.h"
#include "utils/phy_parameters.h"

class Modulation
{
private:
	float _freq;
	float _phase;
	float _amp;
	float _carrier_freq;
	float _sample_rate;

	void calc_step();

protected:
	void setPhase(float phase) { _phase = phase; }
	void setFrequency(float freq) { _freq = freq; }
	void setCarrierFrequency(float freq) { _carrier_freq = freq; }
	void setSampleRate(float sample_rate) { _sample_rate = sample_rate; }
	void setAmp(float amp) { _amp = amp; }
	bool modulate(std::complex<short> *samples, size_t len);

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

	float centralFrequency() const { return _freq; }
	virtual std::string toString() const = 0;

	virtual bool genSamples(std::complex<short> **samples, size_t *len,
				const Message &m, const phy_parameters_t &phy,
				float amp) = 0;
};

#endif // MODULATION_H
