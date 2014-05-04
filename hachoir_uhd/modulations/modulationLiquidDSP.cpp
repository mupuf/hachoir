#ifdef HAS_LIBLIQUID

#include "modulationLiquidDSP.h"
#include "utils/regulation.h"

std::complex<short> ModulationLiquidDSP::toShortComplex(std::complex<float> sample) const
{
	return std::complex<short>(sample.real() * _amp, sample.imag() * _amp);
}

ModulationLiquidDSP::ModulationLiquidDSP(modulation_scheme scheme,
					 float centralFreq, float bauds) :
					 _scheme(scheme),
					 _centralFreq(centralFreq),
					 _bauds(bauds)
{
}

std::string ModulationLiquidDSP::toString() const
{
	char phy_params[150];
	const char *name;
	RegulationBand band;
	size_t channel = -1;

	if (liquid_modem_is_psk(_scheme))
		name = "PSK";
	else if (liquid_modem_is_dpsk(_scheme))
		name = "DPSK";
	else if (liquid_modem_is_ask(_scheme))
		name = "ASK";
	else if (liquid_modem_is_qam(_scheme))
		name = "QAM";
	else if (liquid_modem_is_apsk(_scheme))
		name = "APSK";
	else if (_scheme == LIQUID_MODEM_OOK)
		name = "OOK";
	else
		name = "UNK";

	if (regDB.bandAtFrequencyRange(_centralFreq, _centralFreq, band))
		band.frequencyIsInBand(_centralFreq, &channel);

	snprintf(phy_params, sizeof(phy_params),
		"%i-%s, freq = %.03f MHz (chan = %u), %f bauds",
		modem_get_bps(mod), name, _centralFreq / 1000000.0,
		channel, _bauds);

	return std::string(phy_params);
}

float ModulationLiquidDSP::centralFrequency() const
{
	return _centralFreq;
}

float ModulationLiquidDSP::channelWidth() const
{
	return 20e3;
}

bool ModulationLiquidDSP::prepareMessage(const Message &m,
					 const phy_parameters_t &phy,
					 float amp)
{
	size_t sps = phy.sample_rate / _bauds; // samples per symbols
	size_t filterDelay = 9; // TODO: investigate!
	float beta = 0.2f; // TODO: investigate!
	//double tx_resamp_rate = phy.sample_rate / (sps * phy.IF_bw);
	size_t bps;

	if (sps < 1 /*|| tx_resamp_rate < 1.5*/)
		return false;

	mod = modem_create(_scheme);
	bps = modem_get_bps(mod);
	mfinterp = firinterp_crcf_create_rnyquist(LIQUID_RNYQUIST_RRC, sps,
						  filterDelay, beta, 0);
	//resamp = msresamp_crcf_create(tx_resamp_rate, 60.0f);

	size_t symbCount = m.symbolCount(bps);
	for (size_t i = 0; i < symbCount; i++)
		_symbols.push_back(m.symbolAt(i, bps));

	_phy = phy;
	_amp = amp;

	return true;
}

void ModulationLiquidDSP::getNextSamples(std::complex<short> *samples,
					 size_t *len)
{
	size_t sps = _phy.sample_rate / _bauds; // samples per symbols
	//double tx_resamp_rate = _phy.sample_rate / (sps * _phy.IF_bw);
	std::complex<float> symbol;
	std::complex<float> symbolInterp[sps];
	//std::complex<float> symbolResamp[(size_t)ceilf(sps * tx_resamp_rate)];
	unsigned int nwo = 0;
	size_t o = 0;

	/* copy the data from the previous run */
	while (o < *len && !_remainingSamples.empty()) {
		samples[o++] = toShortComplex(_remainingSamples.at(0));
		_remainingSamples.pop_front();
	}

	/* read the next symbols and add them to the samples buffer */
	while (o < *len && !_symbols.empty()) {
		size_t nextSymbol = _symbols.at(0);
		_symbols.pop_front();
		nwo = 0;

		modem_modulate(mod, nextSymbol, &symbol);
		firinterp_crcf_execute(mfinterp, symbol, symbolInterp);

		while (o < *len && nwo < sps)
			samples[o++] = toShortComplex(symbolInterp[nwo++]);

		/* TODO: move the buffers to the right center frequency */

		/*msresamp_crcf_execute(resamp, symbolInterp, 1, symbolResamp, &nw);
		while (o < *len && nwo < nw)
			samples[o++] = toShortComplex(symbolResamp[nwo++]);*/
	}

	/* add the remaining samples to the buffer */
	while (nwo < sps)
		_remainingSamples.push_back(symbolInterp[nwo++]);

	/* we finished this burst, tell it is! */
	if (o < *len)
		*len = o;
}

#endif
