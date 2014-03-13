#include "manchester.h"

bool Manchester::decode(const Message &in, Message &out)
{
	if (in.size() % 2 != 0)
		return false;

	for(size_t i = 0; i < in.size() - 1; i+=2) {
		if (in[i] == 0 && in[i + 1] == 1)
			out.addBit(false);
		else if (in[i] == 1 && in[i + 1] == 0)
			out.addBit(true);
		else {
			out.clear();
			return false;
		}
	}

	return true;
}

