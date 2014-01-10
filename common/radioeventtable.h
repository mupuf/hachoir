#ifndef RADIOEVENTTABLE_H
#define RADIOEVENTTABLE_H

#include "absoluteringbuffer.h"
#include "ret_entry.h"

#include <list>

class RadioEventTable
{
	uint64_t _currentComID;

	std::list< std::shared_ptr<RetEntry> > _activeComs;
	AbsoluteRingBuffer< RetEntry > _finishedComs;

	/* detection-related */
	uint32_t _endOfTransmissionDelay;
	uint32_t _minimumTransmissionLength;
	uint64_t _tmp_timeNs;
	bool fuzzyCompare(uint32_t a, uint32_t b, int32_t maxError);

	/* Serialization-related */
	enum RadioEventType { ACTIVE_COM = 0, FINISHED_COM = 1, PACKET_END = 2 };
	char *_toStringBuf;
	size_t _toStringBufSize;
	bool toStringBufferReserve(size_t offset, size_t additional);
	bool addCommunicationToString(size_t &offset, RetEntry *entry);
public:
	/* temp, to be moved back to private after the demo */
	uint64_t trueDetection;
	uint64_t totalDetections;

	RadioEventTable(size_t ringSize, uint32_t endOfTransmissionDelay = 0,
			uint32_t minimumTransmissionLength = 0);
	~RadioEventTable();

	void startAddingCommunications(uint64_t timeNs);

	/* Access the current communications */
	const std::list< std::shared_ptr<RetEntry> > &activeCommunications() const { return _activeComs; }
	void updateTransmission(RetEntry *entry, uint64_t frequencyStart,
				uint64_t frequencyEnd,
				int8_t pwr);

	/* add a new communication */
	void addCommunication(uint64_t frequencyStart, uint64_t frequencyEnd,
			      int8_t pwr);
	void stopAddingCommunications();

	bool toString(char **buf, size_t *len);
	bool updateFromString(const char *buf, size_t len);

	std::vector< std::shared_ptr<RetEntry> > fetchEntries(uint64_t timeStart, uint64_t timeEnd);
	RetEntry *findMatchInActiveCommunications(uint32_t frequencyStart,
					      uint32_t frequencyEnd,
					      int8_t pwr);
};

#endif // RADIOEVENTTABLE_H
