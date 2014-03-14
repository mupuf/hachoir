#ifndef OOK_H
#define OOK_H

#include "demodulator.h"
#include "constellation.h"

class OOK : public Demodulator
{
	struct state {
		std::vector <uint64_t> data;
		std::vector<ConstellationPoint> points;
		size_t bps;
	} _on, _off;

	std::string _phy_params;

	uint8_t getBPS(const Constellation &constellation, state &st);
	bool mapSymbol(Message &m, state &st, size_t len);
public:
	OOK();

	uint8_t likeliness(const burst_sc16_t * const burst);
	std::vector<Message> demod(const burst_sc16_t * const burst);
};

#endif // OOK_H
