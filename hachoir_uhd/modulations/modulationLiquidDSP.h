#if !defined(MODULATIONLIQUIDDSP_H) && defined(HAS_LIBLIQUID)
#define MODULATIONLIQUIDDSP_H

#include "modulation.h"
#include <liquid/liquid.h>

#include <deque>

class ModulationLiquidDSP : public Modulation
{
	modulation_scheme _scheme;
	float _centralFreq;
	float _bauds;
	size_t _bps;
	bool _isValid;

	modem mod;
	firinterp_crcf mfinterp;
	msresamp_crcf resamp;

	std::deque<size_t> _symbols;
	phy_parameters_t _phy;
	float _amp;

	std::deque< std::complex<float> > _remainingSamples;

	std::complex<short> toShortComplex(std::complex<float> sample) const;

public:
	ModulationLiquidDSP(modulation_scheme scheme, float centralFreq, float bauds);

	std::string toString() const;

	float centralFrequency() const;
	float channelWidth() const;

	bool prepareMessage(const Message &m, const phy_parameters_t &phy,
			    float amp);

	void getNextSamples(std::complex<short> *samples, size_t *len);
};

#endif // MODULATIONLIQUIDDSP_H
