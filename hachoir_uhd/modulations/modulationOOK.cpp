#include "modulationOOK.h"
#include "utils/regulation.h"

ModulationOOK::ModulationOOK(float centralFreq, const SymbolOOK &ON_Symbol,
			     const SymbolOOK &OFF_Symbol, const SymbolOOK &STOP_Symbol) :
	_centralFreq(centralFreq), _ON_Symbol(ON_Symbol), _OFF_Symbol(OFF_Symbol),
	_STOP_Symbol(STOP_Symbol)
{

}

std::string ModulationOOK::toString() const
{
	char phy_params[150];
	RegulationBand band;
	size_t channel = -1;

	if (regDB.bandAtFrequencyRange(_centralFreq, _centralFreq, band))
		band.frequencyIsInBand(_centralFreq, &channel);

	snprintf(phy_params, sizeof(phy_params),
		"OOK, ON(%u bps) = { %.01f µs, %.01f µs }, OFF(%u bps) = { %.01f µs, %.01f µs }, "
		 "STOP = { %.01f µs }, freq = %.03f MHz (chan = %u)",
		_ON_Symbol.bitsPerSymbol(), _ON_Symbol.bit0_us(), _ON_Symbol.bit1_us(),
		_OFF_Symbol.bitsPerSymbol(), _OFF_Symbol.bit0_us(), _OFF_Symbol.bit1_us(),
		 _STOP_Symbol.bit0_us(), _centralFreq / 1000000.0, channel);

	return std::string(phy_params);
}

bool ModulationOOK::prepareMessage(const Message &m, const phy_parameters_t &phy,
		     float amp)
{
	size_t symbol_us = 0;

	/* TODO: Use the len parameter to know how wide the symbol will be in
	 * the frequency spectrum. Need to talk to Guillaume!
	 */
	if (_centralFreq > (phy.central_freq + phy.sample_rate / 2.0) ||
	    _centralFreq < (phy.central_freq - phy.sample_rate / 2.0))
		return false;

	resetState();

	setFrequency(_centralFreq);
	setCarrierFrequency(phy.central_freq);
	setSampleRate(phy.sample_rate);
	setPhase(0);

	bool isOn = true;
	for (size_t i = 0; i < m.size(); i++) {
		if (isOn)
			_ON_Symbol.symbol(m[i], symbol_us);
		else
			_OFF_Symbol.symbol(m[i], symbol_us);

		setAmp(amp * isOn);
		genSymbol(symbol_us);

		isOn = !isOn;
	}

	// stop bit
	setAmp(amp * isOn);
	genSymbol(_STOP_Symbol.bit0_us());

	endMessage(m.repeatCount());

	return true;
}
