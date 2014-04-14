#include "modulationOOK.h"
#include "utils/regulation.h"

ModulationOOK::ModulationOOK(float centralFreq, const SymbolOOK &ON_Symbol,
			     const SymbolOOK &OFF_Symbol, const SymbolOOK &STOP_Symbol) :
	Modulation(centralFreq), _ON_Symbol(ON_Symbol), _OFF_Symbol(OFF_Symbol),
	_STOP_Symbol(STOP_Symbol)
{

}

std::string ModulationOOK::toString() const
{
	char phy_params[150];
	RegulationBand band;
	size_t channel = -1;

	if (regDB.bandAtFrequencyRange(centralFrequency(), centralFrequency(), band))
		band.frequencyIsInBand(centralFrequency(), &channel);

	snprintf(phy_params, sizeof(phy_params),
		"OOK, ON(%u bps) = { %.01f µs, %.01f µs }, OFF(%u bps) = { %.01f µs, %.01f µs }, "
		 "STOP = { %.01f µs }, freq = %.03f MHz (chan = %u)",
		_ON_Symbol.bitsPerSymbol(), _ON_Symbol.bit0_us(), _ON_Symbol.bit1_us(),
		_OFF_Symbol.bitsPerSymbol(), _OFF_Symbol.bit0_us(), _OFF_Symbol.bit1_us(),
		 _STOP_Symbol.bit0_us(), centralFrequency() / 1000000.0, channel);

	return std::string(phy_params);
}

bool ModulationOOK::genSamples(std::complex<short> **samples, size_t *len, const Message &m,
			       float carrier_freq, float sample_rate, float amp)
{
	setCarrierFrequency(carrier_freq);
	setSampleRate(sample_rate);
	setPhase(0);

	size_t allocated_len = 819200, offset = 0;
	*samples = new std::complex<short>[allocated_len];

	bool isOn = true;
	for (size_t i = 0; i < m.size(); i++) {
		size_t symbol_us;

		if (isOn)
			_ON_Symbol.symbol(m[i], symbol_us);
		else
			_OFF_Symbol.symbol(m[i], symbol_us);

		setAmp(amp * isOn);

		size_t symbol_len = symbol_us * sample_rate / 1000000;
		modulate((*samples) + offset, symbol_len);
		offset += symbol_len;

		isOn = !isOn;
	}

	// stop bit
	size_t symbol_len = _STOP_Symbol.bit0_us() * sample_rate / 1000000;
	setAmp(amp * isOn);
	modulate((*samples) + offset, symbol_len);
	offset += symbol_len;

	*len = offset;

	return true;
}
