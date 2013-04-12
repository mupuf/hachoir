#ifndef RADIOEVENTTABLE_H
#define RADIOEVENTTABLE_H

#include "absoluteringbuffer.h"
#include "ret_entry.h"

#include <list>

class RadioEventTable
{
	uint64_t _currentComID;

	AbsoluteRingBuffer< RetEntry > _finishedComs;
	std::list<RetEntry*> _activeComs;

	/* parameters */
	uint32_t _endOfTransmissionDelay;
	uint32_t _minimumTransmissionLength;


	/* current information */
	uint64_t _tmp_timeNs;
	uint64_t falseDetection;
	uint64_t totalDetections;

	bool fuzzyCompare(uint32_t a, uint32_t b, int32_t maxError);
	RetEntry *findMatch(uint32_t frequencyStart,
					      uint32_t frequencyEnd,
					      int8_t pwr);
public:
	RadioEventTable(size_t ringSize);

	void startAddingCommunications(uint64_t timeNs);
	void addCommunication(uint32_t frequencyStart, uint32_t frequencyEnd,
			      int8_t pwr);
	void stopAddingCommunications();
};

#endif // RADIOEVENTTABLE_H
