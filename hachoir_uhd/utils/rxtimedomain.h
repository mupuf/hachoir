#ifndef RXTIMEDOMAIN_H
#define RXTIMEDOMAIN_H

#include <stddef.h>

#include "utils/phy_parameters.h"
#include "utils/message.h"
#include "utils/burst.h"

typedef bool(*RXTimeDomainMessageCallback)(const Message &msg,
					   phy_parameters_t &phy,
					   void *userData);

class RXTimeDomain
{
	enum rx_state_t {
		LISTEN = 0,
		COALESCING = 1,
		RX = 2
	};

	phy_parameters_t _phy;
	Burst burst;

	RXTimeDomainMessageCallback _userCb;
	void *_userData;

	std::complex<short> _DC_offset;

	void process_dump_samples(std::complex<short> &sample, float mag,
				  float noise_cur_max, float com_thrs,
				  rx_state_t state);
	void burst_dump_samples(const Burst &burst);
	bool process_burst(Burst &burst);

public:
	RXTimeDomain(RXTimeDomainMessageCallback cb = NULL, void *userData = NULL);

	phy_parameters_t phyParameters();
	void setPhyParameters(const phy_parameters_t &phy); /* calls reset */

	void reset();

	bool processSamples(uint64_t time_us, std::complex<short> *samples,
			     size_t count);

	std::complex<short> DC_offset() const { return _DC_offset; }
};

#endif // RXTIMEDOMAIN_H
