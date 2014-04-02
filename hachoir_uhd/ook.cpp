#include "ook.h"
#include <iostream>

#include "com_decode.h"
#include "regulation.h"

OOK::OOK()
{
}

uint8_t OOK::getBPS(const Constellation &constellation, state &st)
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

	// make sure most the two symbols are really common and that the least likely bit is as least 20% probable
	if (c[0].proba + c[1].proba > 0.7 && c[1].proba >= 0.1 * (c[0].proba + c[1].proba)) {
		score = 255;
	}

	// make sure the positions are multiple
	/*if ((c[0].pos * 1.9 < c[1].pos && c[0].pos * 2.1 > c[1].pos) ||
	    (c[1].pos * 1.9 < c[0].pos && c[1].pos * 2.1 > c[0].pos)) {
		score += 127;
	}*/

	if (score >= 128) {
		st.bps = 1;
	} else {
		st.bps = 0;
	}

	return score;
}

bool OOK::mapSymbol(Message &m, state &st, size_t len)
{
	if (st.bps == 1) {
		float min = st.points[0].pos < st.points[1].pos ? st.points[0].pos : st.points[1].pos;
		float max = st.points[0].pos > st.points[1].pos ? st.points[0].pos : st.points[1].pos;
		float dist_max = (max - min) / 2;
		if (fabs(len - min) < dist_max)
			m.addBit(false);
		else if (fabs(len - max) < dist_max)
			m.addBit(true);
		else
			return false;
	} /*else if (st.bps == 2) {
		size_t thrs0 = st.time_min + 1 * ((st.time_max - st.time_min) / 4);
		size_t thrs1 = st.time_min + 2 * ((st.time_max - st.time_min) / 4);
		size_t thrs2 = st.time_min + 3 * ((st.time_max - st.time_min) / 4);

		if (len < thrs0) {
			m.addBit(false);
			m.addBit(false);
		} else if (len < thrs1) {
			m.addBit(false);
			m.addBit(true);
		} else if (len < thrs2) {
			m.addBit(true);
			m.addBit(true);
		} else {
			m.addBit(true);
			m.addBit(false);
		}
	} else
		return false;*/

	return true;
}

uint8_t OOK::likeliness(const burst_sc16_t * const burst)
{
	Constellation cOn, cOff;
	uint8_t score = 0;

	float central_freq, freq_std;
	RegulationBand band;
	size_t channel;

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

	cOn.clusterize(0.1);
	cOff.clusterize(0.1);

	// calculate the number of bits per symbols
	uint8_t score_on = getBPS(cOn, _on);
	uint8_t score_off = getBPS(cOff, _off);

	// calculate the score
	score = ((score_on > score_off) ? score_on : score_off) / 2;
	if (burst->sub_bursts.size() > 30)
		score += 127;

	freq_get_avr(burst, central_freq, freq_std);
	if (regDB.bandAtFrequencyRange(central_freq, central_freq, band))
		band.frequencyIsInBand(central_freq, &channel);

	// store the PHY parameters in a single string
	char phy_params[150];
	snprintf(phy_params, sizeof(phy_params),
		"OOK, ON(%u bps) = { %.01f (p=%.02f), %.01f (p=%.02f) }, OFF(%u bps) = { %.01f (p=%.02f), %.01f (p=%.02f) }, freq = %.03f MHz (chan = %u)",
		_on.bps, _on.points[0].pos, _on.points[0].proba, _on.points[1].pos, _on.points[1].proba,
		_off.bps, _off.points[0].pos, _off.points[0].proba, _off.points[1].pos, _off.points[1].proba,
		central_freq, channel);
	_phy_params = phy_params;

	return score;
}

std::vector<Message> OOK::demod(const burst_sc16_t * const burst)
{
	std::vector<Message> msgs;
	Message m(_phy_params);

	// TODO: Detect the end of the message to start another one!
	for (size_t i = 0; i < _on.data.size() - 1; i++) {
		if (!mapSymbol(m, _on, _on.data[i])) {
			std::cout << "OOK: Unknown ON symbol " << _on.data[i] << std::endl;
			if (m.size() > 0) {
				msgs.push_back(m);
				m.clear();
			}
		}
		if (!mapSymbol(m, _off, _off.data[i])) {
			std::cout << "OOK: Unknown OFF symbol " << _off.data[i] << std::endl;
			if (m.size() > 0) {
				msgs.push_back(m);
				m.clear();
			}
		}
	}
	if (!mapSymbol(m, _on, _on.data[_on.data.size() - 1]))
			std::cout << "OOK: Unknown ON symbol " << _on.data[_on.data.size() - 1] << std::endl;

	msgs.push_back(m);

	return msgs;
}
