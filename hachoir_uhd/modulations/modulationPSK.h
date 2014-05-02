#ifndef MODULATIONPSK_H
#define MODULATIONPSK_H

#include "modulation.h"
#include <complex>

class ModulationPSK : public Modulation
{
public:
	float _centralFreq;
	float _bauds;
	size_t _bps;
	bool _dpsk;

public:
	ModulationPSK(float centralFreq, float bauds, size_t bps, bool dpsk = true);
	std::string toString() const;

	float centralFrequency() const;
	float channelWidth() const;

	bool prepareMessage(const Message &m, const phy_parameters_t &phy,
			     float amp);
};

#endif // MODULATIONFSK_H
