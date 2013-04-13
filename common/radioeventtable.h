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

	/* temp */
	uint64_t _tmp_timeNs;
	char *_toStringBuf;
	size_t _toStringBufSize;

	bool fuzzyCompare(uint32_t a, uint32_t b, int32_t maxError);
	RetEntry *findMatch(uint32_t frequencyStart,
					      uint32_t frequencyEnd,
					      int8_t pwr);
public:
	uint64_t trueDetection;
	uint64_t totalDetections;

	RadioEventTable(size_t ringSize);

	void startAddingCommunications(uint64_t timeNs);
	void addCommunication(uint32_t frequencyStart, uint32_t frequencyEnd,
			      int8_t pwr);
	void stopAddingCommunications();

	void toString(const char **buf, size_t *len);
	bool updateFromString(const char *buf, size_t len);
};

#endif // RADIOEVENTTABLE_H
