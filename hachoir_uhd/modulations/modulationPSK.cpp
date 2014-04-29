#include "modulationPSK.h"
#include "utils/regulation.h"

#include <iostream>

ModulationPSK::ModulationPSK(float centralFreq, float bauds, size_t bps) :
	_centralFreq(centralFreq), _bauds(bauds), _bps(bps)

{
}

std::string ModulationPSK::toString() const
{
	char phy_params[150];
	RegulationBand band;
	size_t channel = -1;

	if (regDB.bandAtFrequencyRange(_centralFreq, _centralFreq, band))
		band.frequencyIsInBand(_centralFreq, &channel);

	snprintf(phy_params, sizeof(phy_params),
		"%i-PSK, freq = %.03f MHz (chan = %u)",
		_bps, _centralFreq / 1000000.0, channel);

	return std::string(phy_params);
}

float ModulationPSK::centralFrequency() const
{
	return _centralFreq;
}

float ModulationPSK::channelWidth() const
{
	/* TODO: Use the len parameter to know how wide the symbol will be in
	 * the frequency spectrum. Need to talk to Guillaume!
	 */
	return 20e3;
}

bool ModulationPSK::prepareMessage(const Message &m, const phy_parameters_t &phy,
		     float amp)
{
	size_t symbol_us = 1000000.0 / _bauds / _bps;

	resetState();

	setCarrierFrequency(phy.central_freq);
	setFrequency(_centralFreq);
	setSampleRate(phy.sample_rate);
	setAmp(amp);

	for (size_t i = 0; i < m.size(); i++) {
		float phaseDiff;
		if (_bps == 1) {
			if (m[i])
				phaseDiff = M_PI;
			else
				phaseDiff = -M_PI;
		} else {
			std::cerr << "Unknown modulation!" << std::endl;
			return false;
		}

		setPhaseDiff(phaseDiff);
		genSymbol(symbol_us);
	}

	// stop bit
	setAmp(0.0);
	genSymbol(100);

	endMessage(m.repeatCount());

	return true;
}
