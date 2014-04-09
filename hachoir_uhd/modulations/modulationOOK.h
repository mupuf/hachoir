#ifndef MODULATIONOOK_H
#define MODULATIONOOK_H

#include "modulation.h"

class ModulationOOK : public Modulation
{
public:
	class SymbolOOK
	{
		size_t _bps;
		float _bit_0;
		float _bit_1;

	public:
		SymbolOOK() : _bps(0), _bit_0(-1.0), _bit_1(-1.0) {}
		SymbolOOK(float time_us) : _bps(0), _bit_0(time_us), _bit_1(-1.0) {}
		SymbolOOK(float bit0_us, float bit1_us) : _bps(1), _bit_0(bit0_us), _bit_1(bit1_us) {}
		SymbolOOK(const SymbolOOK &sOOK) : _bps(sOOK._bps), _bit_0(sOOK._bit_0), _bit_1(sOOK._bit_1) {}

		size_t bitsPerSymbol() const { return _bps; }

		float bit0_us() const { return _bit_0; }
		float bit1_us() const { return _bit_1; }
	};

private:
	SymbolOOK _ON_Symbol;
	SymbolOOK _OFF_Symbol;
	SymbolOOK _STOP_Symbol;

public:
	ModulationOOK(float centralFreq, const SymbolOOK &ON_Symbol,
		      const SymbolOOK &OFF_Symbol, const SymbolOOK &STOP_Symbol);
	std::string toString() const;
};

#endif // MODULATIONOOK_H
