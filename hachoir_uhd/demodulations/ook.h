#ifndef OOK_H
#define OOK_H

#include "demodulator.h"
#include "utils/constellation.h"
#include "modulations/modulationOOK.h"

class OOK : public Demodulator
{
	struct state {
		std::vector <uint64_t> data;
		std::vector<ConstellationPoint> points;
		size_t bps;
		ModulationOOK::SymbolOOK symbol;
	} _on, _off;

	std::string _phy_params;

	uint8_t getBPS(const Constellation &constellation, state &st, float sample_rate);
	bool mapSymbol(Message &m, state &st, size_t len);
public:
	OOK();

	uint8_t likeliness(const burst_t * const burst);
	std::vector<Message> demod(const burst_t * const burst);
	std::string modulationString() const;
};

#endif // OOK_H
