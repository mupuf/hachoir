#ifndef MODULATIONFSK_H
#define MODULATIONFSK_H

#include "modulation.h"
#include <complex>

class ModulationFSK : public Modulation
{
public:
	float _centralFreq;
	float _bauds;
	float _freqSpacing;
	size_t _bps;

public:
	ModulationFSK(float centralFreq, float bauds, float freqSpacing, size_t bps);
	std::string toString() const;

	float centralFrequency() const;
	float channelWidth() const;

	bool prepareMessage(const Message &m, const phy_parameters_t &phy,
			     float amp);
};

#endif // MODULATIONFSK_H
