#ifndef DEMODULATOR_H
#define DEMODULATOR_H

#include <vector>

#include "utils/com_decode.h"
#include "utils/message.h"

class Demodulator
{
public:
	virtual uint8_t likeliness(const burst_sc16_t * const burst) = 0;
	virtual std::vector<Message> demod(const burst_sc16_t * const burst) = 0;
	virtual std::string modulationString() const = 0;
};

#endif // DEMODULATOR_H
