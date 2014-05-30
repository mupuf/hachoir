#include "band.h"

Band::Band() { }

Band::Band(float start, float end) : _start(start), _end(end)
{
}

bool Band::isBandIncluded(Band b) const
{
	return b.start() >= start() && b.end() <= end();
}
