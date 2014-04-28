#include "modulationFSK.h"
#include "utils/regulation.h"

#include <iostream>

ModulationFSK::ModulationFSK(float centralFreq, float bauds, float freqSpacing,
			     size_t bps) :
	_centralFreq(centralFreq), _bauds(bauds), _freqSpacing(freqSpacing),
	_bps(bps)

{

}

std::string ModulationFSK::toString() const
{
	char phy_params[150];
	RegulationBand band;
	size_t channel = -1;

	if (regDB.bandAtFrequencyRange(_centralFreq, _centralFreq, band))
		band.frequencyIsInBand(_centralFreq, &channel);

	snprintf(phy_params, sizeof(phy_params),
		"%i-FSK, spacing = %.03f kHz, freq = %.03f MHz (chan = %u)",
		_bps, _freqSpacing / 1000.0, _centralFreq / 1000000.0, channel);

	return std::string(phy_params);
}

float ModulationFSK::centralFrequency() const
{
	return _centralFreq;
}

float ModulationFSK::channelWidth() const
{
	/* TODO: Use the len parameter to know how wide the symbol will be in
	 * the frequency spectrum. Need to talk to Guillaume!
	 */
	return 20e3;
}

bool ModulationFSK::prepareMessage(const Message &m, const phy_parameters_t &phy,
		     float amp)
{
	size_t symbol_us = 1000000.0 / _bauds / _bps;
	float freq_offset = 0.0;

	resetState();

	setCarrierFrequency(phy.central_freq);
	setSampleRate(phy.sample_rate);
	setPhase(0);
	setAmp(amp);

	for (size_t i = 0; i < m.size(); i++) {
		if (_bps == 1) {
			if (!m[i])
				freq_offset = -_freqSpacing / 2.0;
			else
				freq_offset = _freqSpacing / 2.0;
		} else {
			std::cerr << "Unknown modulation!" << std::endl;
			return false;
		}

		setFrequency(_centralFreq + freq_offset);
		genSymbol(symbol_us);
	}

	// stop bit
	setAmp(0.0);
	genSymbol(100);

	endMessage(m.repeatCount());

	return true;
}
