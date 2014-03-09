#ifndef OOK_H
#define OOK_H

#include "demodulator.h"

class OOK : public Demodulator
{
	struct state {
		std::vector <uint64_t> data;
		size_t time_min, time_max;
		size_t bps;
	} _on, _off;

	size_t findBitsPerSymbol(size_t time_min, size_t time_max);
	bool mapSymbol(Message &m, state &st, size_t len);
public:
	OOK();

	uint8_t likeliness(const burst_sc16_t * const burst);
	Message &demod(const burst_sc16_t * const burst);
};

#endif // OOK_H
