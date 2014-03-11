#include "ook.h"
#include <iostream>

OOK::OOK()
{
}

uint8_t OOK::getBPS(const Constellation &constellation, state &st)
{
	ConstellationPoint c[2];
	uint8_t score = 0;

	for (size_t i = 0; i < sizeof(c) / sizeof(ConstellationPoint); i++)
		c[i] = constellation.mostProbabilisticPoint(i);

	int i = 0;
	ConstellationPoint cp;
	do {
		cp = constellation.mostProbabilisticPoint(i);
		std::cout << "[" << cp.pos << ", p=" << cp.proba << "] ";
		i++;
	} while (cp.proba > 0.0);
	std::cout << std::endl;


	st.points.push_back(c[0]);
	st.points.push_back(c[1]);

	// make sure most the two symbols are really common and that the least likely bit is as least 20% probable
	if (c[0].proba + c[1].proba > 0.7 && c[1].proba >= 0.2 * (c[0].proba + c[1].proba)) {
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
			std::cout << "OOK: Unknown symbol " << len << std::endl;
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

	// calculate the number of bits per symbols
	uint8_t score_on = getBPS(cOn, _on);
	uint8_t score_off = getBPS(cOff, _off);

	// calculate the score
	score = ((score_on > score_off) ? score_on : score_off) / 2;
	if (burst->sub_bursts.size() > 30)
		score += 127;


	// get the frequency of the signal
	uint64_t sum_cnt = 0, count_cnt = 0;
	size_t last_crossing = 0;

	for (size_t b = 0; b < burst->sub_bursts.size(); b++) {
		std::complex<short> *start = &burst->samples[burst->sub_bursts[b].start];

		/*fprintf(stderr, "%u: samples = %p, start = %p, idx = %u, burst->sub_bursts[b].len = %u, len = %u\n",
			b, burst->samples, start, burst->sub_bursts[b].start,
			burst->sub_bursts[b].len, burst->len);*/

		if (burst->sub_bursts[b].len == 0)
			continue;

		last_crossing = 0;
		for (size_t i = 0; i < burst->sub_bursts[b].len - 1; i++) {
			//fprintf(stderr, "i = %u\n", i);
			if ((start[i].real() > 0 && start[i + 1].real() <= 0) ||
			    (start[i].real() < 0 && start[i + 1].real() >= 0)) {
				if (last_crossing > 0) {
					sum_cnt += (i - last_crossing);
					count_cnt++;
				}
				last_crossing = i;
			}
		}
	}

	// store the PHY parameters in a single string
	char phy_params[150];
	float sample_avr = sum_cnt * 1.0 / count_cnt;
	float freq_offset = (burst->phy.sample_rate / sample_avr) / 2;
	snprintf(phy_params, sizeof(phy_params),
		"OOK, ON(%u bps) = { %.01f (p=%.02f), %.01f (p=%.02f) }, OFF(%u bps) = { %.01f (p=%.02f), %.01f (p=%.02f) }, freq = %.03f MHz or %.03f MHz",
		_on.bps, _on.points[0].pos, _on.points[0].proba, _on.points[1].pos, _on.points[1].proba,
		_off.bps, _off.points[0].pos, _off.points[0].proba, _off.points[1].pos, _off.points[1].proba,
		(burst->phy.central_freq + freq_offset) / 1000000.0,
		(burst->phy.central_freq - freq_offset) / 1000000.0);
	_phy_params = phy_params;

	return score;
}

Message OOK::demod(const burst_sc16_t * const burst)
{
	Message m(_phy_params);

	// TODO: Detect the end of the message to start another one!
	for (size_t i = 0; i < _on.data.size() - 1; i++) {
		mapSymbol(m, _on, _on.data[i]);
		mapSymbol(m, _off, _off.data[i]);
	}
	mapSymbol(m, _on, _on.data[_on.data.size() - 1]);

	return m;
}
