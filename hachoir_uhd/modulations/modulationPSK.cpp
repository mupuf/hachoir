#include "modulationPSK.h"
#include "utils/regulation.h"

#include <iostream>

ModulationPSK::ModulationPSK(float centralFreq, float bauds, size_t bps, bool dpsk) :
	_centralFreq(centralFreq), _bauds(bauds), _bps(bps), _dpsk(dpsk)

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
	bool previousBit = false;

	size_t symbol_us = 1000000.0 / _bauds / _bps;

	resetState();

	setCarrierFrequency(phy.central_freq);
	setFrequency(_centralFreq);
	setSampleRate(phy.sample_rate);
	setAmp(amp);

	// start bit
	genSymbol(symbol_us);

	for (size_t i = 0; i < m.size(); i++) {
		float phaseDiff = 0;
		if (_bps == 1) {
			bool bit = m[i];
			if (_dpsk || i == 0 || bit != previousBit) {
				if (bit)
					phaseDiff = M_PI_2;
				else
					phaseDiff = -M_PI_2;
			}
			previousBit = bit;
		} else {
			std::cerr << "Unknown modulation!" << std::endl;
			return false;
		}

		setPhaseDiff(phaseDiff);
		genSymbol(symbol_us);
	}

	// stop bit
	setAmp(0.0);
	genSymbol(symbol_us);

	endMessage(m.repeatCount());

	return true;
}
