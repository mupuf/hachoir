#ifndef RXTIMEDOMAIN_H
#define RXTIMEDOMAIN_H

#include <stddef.h>

#include "utils/phy_parameters.h"
#include "utils/burst.h"

class RXTimeDomain
{
	enum rx_state_t {
		LISTEN = 0,
		COALESCING = 1,
		RX = 2
	};

	phy_parameters_t _phy;
	Burst burst;

	void burst_dump_samples(const Burst &burst);
	bool process_burst(Burst &burst);

public:
	RXTimeDomain();

	phy_parameters_t phyParameters();
	void setPhyParameters(const phy_parameters_t &phy); /* calls reset */

	void reset();

	bool processSamples(uint64_t time_us, std::complex<short> *samples,
			     size_t count);
};

#endif // RXTIMEDOMAIN_H
