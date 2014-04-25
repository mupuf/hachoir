#ifndef DEMODULATOR_H
#define DEMODULATOR_H

#include <vector>

#include "utils/message.h"
#include "utils/burst.h"

class Demodulator
{
public:
	virtual uint8_t likeliness(const Burst &burst) = 0;
	virtual std::vector<Message> demod(const Burst &burst) = 0;
	virtual std::string modulationString() const = 0;
};

#endif // DEMODULATOR_H
