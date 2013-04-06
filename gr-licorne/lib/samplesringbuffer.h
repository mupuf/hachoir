#ifndef SAMPLESRINGBUFFER_H
#define SAMPLESRINGBUFFER_H

#include "ringbuffer.h"

struct RBMarker { uint64_t freq; uint64_t time; };

class SamplesRingBuffer : public RingBuffer<gr_complex, RBMarker>
{
	uint64_t _sampleRate; // samples per second

public:
	SamplesRingBuffer(size_t size, size_t sampleRate) :
		RingBuffer(size), _sampleRate(sampleRate)
	{

	}

	uint64_t timeAtWithMarker(const RBMarker &marker, size_t offset)
	{
		static float sampleTime = 1/_sampleRate;
		return marker.time + offset * sampleTime;
	}

	uint64_t timeAtMarker(uint64_t markerPos)
	{
		RBMarker marker;

		if (getMarker(markerPos, &marker))
			return timeAtWithMarker(marker, 0);
		else
			return 0;
	}

	uint64_t timeAt(uint64_t pos)
	{
		RBMarker marker;
		uint64_t markerPos;

		if (findMarker(pos, &marker, &markerPos))
			return timeAtWithMarker(marker, pos - markerPos);
		else
			return 0;
	}
};

#endif // SAMPLESRINGBUFFER_H
