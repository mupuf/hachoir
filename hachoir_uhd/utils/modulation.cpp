#include "modulation.h"

Modulation::Modulation(size_t baudrate) : _baudrate(baudrate)
{
}

/*ModulationOOK::ModulationOOK(size_t baudrate, SymbolOOK ON_Symbol, SymbolOOK OFF_Symbol) :
	Modulation(baudrate), _ON_Symbol(ON_Symbol), _OFF_Symbol(OFF_Symbol)
{

}

std::string ModulationOOK::toString() const
{
	return std::string();
}*/
