#ifndef MANCHESTER_H
#define MANCHESTER_H

#include "message.h"

class Manchester
{
public:
	static bool decode(const Message &in, Message &out);
};

#endif // MANCHESTER_H
