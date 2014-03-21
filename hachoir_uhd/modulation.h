#ifndef MODULATION_H
#define MODULATION_H

#include <string>
#include <vector>

class Modulation
{
private:
	size_t _baudrate;

public:
	enum ModulationType {
		UNKNOWN	= 0,
		OOK	= 1,
		FSK	= 2,
		PSK	= 4,
		DPSK	= 8,
		QAM	= 16
	};

	virtual std::string toString() const = 0;

protected:
	Modulation(size_t baudrate);
};

/*class ModulationOOK : public Modulation
{
public:
	class SymbolOOK
	{
		size_t _bitPerSymbol;
		std::vector<float> _value;

	public:
		SymbolOOK(size_t bitPerSymbol) : _bitPerSymbol(bitPerSymbol) {}
		void addValue(float value) { _value.push_back(value); }
		bool isValid() { return _bitPerSymbol == _value.size(); }
	};

private:
	SymbolOOK _ON_Symbol;
	SymbolOOK _OFF_Symbol;

public:

	ModulationOOK(size_t bauds, SymbolOOK ON_Symbol, SymbolOOK OFF_Symbol);
	std::string toString() const;
};*/

#endif // MODULATION_H
