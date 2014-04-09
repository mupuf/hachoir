#include "ook.h"
#include <iostream>

#include "utils/com_decode.h"
#include "utils/regulation.h"

OOK::OOK()
{
}

uint8_t OOK::getBPS(const Constellation &constellation, state &st, float sample_rate)
{
	ConstellationPoint c[2];
	uint8_t score = 0;

	for (size_t i = 0; i < sizeof(c) / sizeof(ConstellationPoint); i++)
		c[i] = constellation.mostProbabilisticPoint(i);

	/*int i = 0;
	ConstellationPoint cp;
	do {
		cp = constellation.mostProbabilisticPoint(i);
		if (cp.proba > 0.0)
			std::cout << cp.toString() << " ";
		i++;
	} while (cp.proba > 0.0);
	std::cout << std::endl;*/


	st.points.push_back(c[0]);
	st.points.push_back(c[1]);

	// make sure the positions are multiple
	float min = st.points[0].pos < st.points[1].pos ? st.points[0].pos : st.points[1].pos;
	float max = st.points[0].pos > st.points[1].pos ? st.points[0].pos : st.points[1].pos;

	// make	sure the 2 most important peaks account for 80%
	if (c[0].proba + c[1].proba >= 0.7 && c[1].proba >= 0.1 * (c[0].proba + c[1].proba) && max >= (min * 1.9)) {
			score = 255;
			st.bps = 1;
	} else if (c[0].proba + c[1].proba >= 0.85 && max >= (min * 1.9) && max <= (min * 2.1)) {
		score = 255;
		st.bps = 1;
	} else
		st.bps = 0;

	if (st.bps == 0)
		st.symbol = ModulationOOK::SymbolOOK(st.points[0].pos * 1000000.0 / sample_rate);
	else if (st.bps == 1)
		st.symbol = ModulationOOK::SymbolOOK(min * 1000000.0 / sample_rate,
						     max * 1000000.0 / sample_rate);

	return score;
}

bool OOK::mapSymbol(Message &m, state &st, size_t len)
{
	float min = st.points[0].pos < st.points[1].pos ? st.points[0].pos : st.points[1].pos;
	float max = st.points[0].pos > st.points[1].pos ? st.points[0].pos : st.points[1].pos;
	float dist_max = (max - min) / 2;

	if (st.bps == 1) {
		if (fabs(len - min) < dist_max)
			m.addBit(false);
		else if (fabs(len - max) < dist_max)
			m.addBit(true);
		else
			return false;
	} else if (st.bps == 0) {
		if (len > st.points[0].posMax * 1.5)
			return false;
	}

	return true;
}

uint8_t OOK::likeliness(const burst_sc16_t * const burst)
{
	Constellation cOn, cOff;
	uint8_t score = 0;

	/* reset the parameters */
	_on.data.clear();
	_off.data.clear();

	// put all the on and off times in two vectors
	uint64_t last_stop = 0;
	for (sub_burst_sc16_t sb : burst->sub_bursts) {
		if (last_stop > 0) {
			size_t off_time = sb.time_start_us - last_stop;
			cOff.addPoint(off_time);
			_off.data.push_back(off_time);
		}

		size_t on_time = sb.time_stop_us - sb.time_start_us;
		cOn.addPoint(on_time);
		_on.data.push_back(on_time);

		last_stop = sb.time_stop_us;
	}

	cOn.clusterize();
	cOff.clusterize();

	//std::cerr << cOn.histogram() << std::endl;

	// calculate the number of bits per symbols
	uint8_t score_on = getBPS(cOn, _on, burst->phy.sample_rate);
	uint8_t score_off = getBPS(cOff, _off, burst->phy.sample_rate);

	// calculate the score
	score = ((score_on > score_off) ? score_on : score_off) / 2;
	if (burst->sub_bursts.size() > 20)
		score += 128;

	return score;
}

std::vector<Message> OOK::demod(const burst_sc16_t * const burst)
{
	std::shared_ptr<Modulation> mod;
	std::vector<Message> msgs;
	float central_freq, freq_std;
	RegulationBand band;
	size_t channel;
	Message m;

	freq_get_avr(burst, central_freq, freq_std);
	if (regDB.bandAtFrequencyRange(central_freq, central_freq, band))
		band.frequencyIsInBand(central_freq, &channel);

	// store the PHY parameters in a single string
	char phy_params[150];
	snprintf(phy_params, sizeof(phy_params),
		"OOK, ON(%u bps) = { %.01f (p=%.02f), %.01f (p=%.02f) }, OFF(%u bps) = { %.01f (p=%.02f), %.01f (p=%.02f) }, freq = %.03f MHz (chan = %u)",
		_on.bps, _on.points[0].pos, _on.points[0].proba, _on.points[1].pos, _on.points[1].proba,
		_off.bps, _off.points[0].pos, _off.points[0].proba, _off.points[1].pos, _off.points[1].proba,
		central_freq / 1000000.0, channel);
	_phy_params = phy_params;

	for (size_t i = 0; i < _on.data.size() - 1; i++) {
		if (!mapSymbol(m, _on, _on.data[i])) {
			std::cerr << "OOK: Unknown ON symbol " << _on.data[i] << std::endl;
			if (m.size() > 0) {
				ModulationOOK::SymbolOOK stopSymbol(_on.data[i] * 1000000.0 / burst->phy.sample_rate);
				mod.reset(new ModulationOOK(central_freq, _on.symbol, _off.symbol, stopSymbol));
				m.setModulation(mod);
				msgs.push_back(m);
				m.clear();
			}
		}
		if (!mapSymbol(m, _off, _off.data[i])) {
			std::cerr << "OOK: Unknown OFF symbol " << _off.data[i] << std::endl;
			if (m.size() > 0) {
				ModulationOOK::SymbolOOK stopSymbol(_off.data[i] * 1000000.0 / burst->phy.sample_rate);
				mod.reset(new ModulationOOK(central_freq, _on.symbol, _off.symbol, stopSymbol));
				m.setModulation(mod);
				msgs.push_back(m);
				m.clear();
			}
		}
	}
	if (!mapSymbol(m, _on, _on.data[_on.data.size() - 1]))
			std::cerr << "OOK: Unknown ON symbol " << _on.data[_on.data.size() - 1] << std::endl;

	if (m.size() > 0) {
		ModulationOOK::SymbolOOK stopSymbol;
		mod.reset(new ModulationOOK(central_freq, _on.symbol, _off.symbol, stopSymbol));
		m.setModulation(mod);
		msgs.push_back(m);
	}

	return msgs;
}

std::string OOK::modulationString() const
{
	return _phy_params;
}
