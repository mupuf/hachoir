#include "ook.h"
#include <iostream>

OOK::OOK()
{
}

size_t OOK::findBitsPerSymbol(size_t time_min, size_t time_max)
{
	float ratio = (float)time_max / time_min;

	if (ratio < 1.75)
		return 0;
	else if (ratio < 2.75)
		return 1;
	else if (ratio < 4.75)
		return 2;
	else
		return 0;
}

bool OOK::mapSymbol(Message &m, state &st, size_t len)
{
	if (st.bps == 1) {
		size_t thrs = st.time_min + (st.time_max - st.time_min) / 2;
		m.addBit(len < thrs);
	} else if (st.bps == 2) {
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
		return false;

	return true;
}

uint8_t OOK::likeliness(const burst_sc16_t * const burst)
{
	uint8_t score;

	/* reset the parameters */
	_on.data.clear();
	_off.data.clear();
	_on.time_min = ~0;
	_off.time_min = ~0;
	_on.time_max = 0;
	_off.time_max = 0;

	// put all the on and off times in two vectors
	uint64_t last_stop = 0;
	for (sub_burst_sc16_t sb : burst->sub_bursts) {
		if (last_stop > 0) {
			size_t off_time = sb.time_start_us - last_stop;

			if (off_time > _off.time_max) _off.time_max = off_time;
			if (off_time < _off.time_min) _off.time_min = off_time;

			_off.data.push_back(off_time);
			//std::cerr << "OOK: add offs: " << off_time << std::endl;
		}

		size_t on_time = sb.time_stop_us - sb.time_start_us;
		if (on_time > _on.time_max) _on.time_max = on_time;
		if (on_time < _on.time_min) _on.time_min = on_time;
		//std::cerr << "OOK: add ons: " << on_time << std::endl;
		_on.data.push_back(on_time);

		last_stop = sb.time_stop_us;
	}

	// calculate the number of bits per symbols
	_on.bps = findBitsPerSymbol(_on.time_min, _on.time_max);
	_off.bps = findBitsPerSymbol(_off.time_min, _off.time_max);

	if (_on.bps == 0 && _off.bps == 0)
		return 0;
	else
		score = 128;

	if (burst->sub_bursts.size() >= 16)
		score += 127;
	else
		score += 8 * burst->sub_bursts.size();

	// get the frequency of the signal
	uint64_t sum_cnt = 0, count_cnt = 0;
	size_t last_crossing = 0;

	for (size_t b = 0; b < burst->sub_bursts.size(); b++) {
		std::complex<short> *start = &burst->samples[burst->sub_bursts[b].start];

		last_crossing = 0;
		for (size_t i = 0; i < burst->sub_bursts[b].len - 1; i++) {
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
		"OOK, ON [%u µs,%u µs] (%u bps), OFF [%u µs, %u µs] (%u bps), freq = %.03f MHz or %.03f MHz",
		_on.time_min, _on.time_max, _on.bps,
		_off.time_min, _off.time_max, _off.bps,
		(burst->phy.central_freq + freq_offset) / 1000000.0,
		(burst->phy.central_freq - freq_offset) / 1000000.0);
	_phy_params = phy_params;

	return score;
}

Message OOK::demod(const burst_sc16_t * const burst)
{
	Message m(_phy_params);

	for (size_t i = 0; i < _on.data.size() - 1; i++) {
		mapSymbol(m, _on, _on.data[i]);
		mapSymbol(m, _off, _off.data[i]);
	}
	mapSymbol(m, _on, _on.data[_on.data.size() - 1]);

	return m;
}
