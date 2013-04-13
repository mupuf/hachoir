#include "radioeventtable.h"

#include "message_utils.h"

bool RadioEventTable::fuzzyCompare(uint32_t a, uint32_t b, int32_t maxError)
{
	int32_t sub = a -b;
	return sub > -maxError && sub < maxError;
}

RetEntry * RadioEventTable::findMatch(uint32_t frequencyStart,
				      uint32_t frequencyEnd,
				      int8_t pwr)
{
	RetEntry * entry;

	uint32_t comWidth = frequencyEnd - frequencyStart;
	uint32_t comCentralFreq = frequencyStart + comWidth / 2;

	std::list<RetEntry *>::iterator it;
	for (it = _activeComs.begin(); it != _activeComs.end(); ++it) {
		entry = *it;

		uint32_t width = entry->frequencyEnd() - entry->frequencyStart();
		uint32_t centralFreq = entry->frequencyStart() + width / 2;

		if (fuzzyCompare(comWidth, width, width / 10) &&
		    fuzzyCompare(comCentralFreq, centralFreq, width / 10))
			return entry;
	}

	return NULL;
}

RadioEventTable::RadioEventTable(size_t ringSize) : _currentComID(0),
	_finishedComs(ringSize), _endOfTransmissionDelay(10000) /* 10µs */,
	_minimumTransmissionLength(50000) /* 50 µs */
{
	trueDetection = totalDetections = 0;
	_toStringBufSize = 1000000; /* 1 MB */
	_toStringBuf = (char *)malloc(_toStringBufSize);

}

#define DEBUG_ADD_COMMUNICATION 0

void RadioEventTable::startAddingCommunications(uint64_t timeNs)
{
	this->_tmp_timeNs = timeNs;
#if DEBUG_ADD_COMMUNICATION
	fprintf(stderr, "%llu: Add new coms!\n", timeNs);
#endif
}

void entryToStderr(const RetEntry *e)
{
	if (e)
	{
		fprintf(stderr, "(id = %llu [%llu, %llu = %u]ns, [%u, %u = %u]kHz)",
			e->id(), e->timeStart(), e->timeEnd(), e->timeEnd() - e->timeStart(),
			e->frequencyStart(), e->frequencyEnd(), e->frequencyEnd() - e->frequencyStart());
	} else
		fprintf(stderr, "(NULL)");
}


void RadioEventTable::addCommunication(uint32_t frequencyStart,
				       uint32_t frequencyEnd,
				       int8_t pwr)
{
	RetEntry * entry = findMatch(frequencyStart, frequencyEnd, pwr);
#if DEBUG_ADD_COMMUNICATION
	fprintf(stderr, "	%s a match for ", entry?"Found":"Didn't find");
	entryToStderr(entry);
	fprintf(stderr, " --> ");
#endif
	if (entry == NULL) {
		/* no corresponding entry found, let's create one! */
		entry = new RetEntry(_currentComID++, _tmp_timeNs, _tmp_timeNs, frequencyStart,
					    frequencyEnd, pwr, RetEntry::UNKNOWN, 0);
		_activeComs.push_back(entry);
#if DEBUG_ADD_COMMUNICATION
		fprintf(stderr, "add new entry: ");
		entryToStderr(entry);
		fprintf(stderr, "\n");
#endif
	} else {
		/* found a match, update it! */
		entry->setTimeEnd(_tmp_timeNs);
		if (entry->frequencyStart() > frequencyStart)
			entry->setFrequencyStart(frequencyStart);
		if (entry->frequencyEnd() < frequencyEnd)
			entry->setFrequencyStart(frequencyStart);
		if (entry->pwr() < pwr)
			entry->setPwr(pwr);
		entry->setDirty(true);
#if DEBUG_ADD_COMMUNICATION
		entryToStderr(entry);
		fprintf(stderr, "\n");
#endif
	}
}

void RadioEventTable::stopAddingCommunications()
{
	RetEntry * entry;

	/* transfer non-ongoing communications to _finishedComs */
	std::list<RetEntry *>::iterator it;
	for (it = _activeComs.begin(); it != _activeComs.end(); ++it) {
		entry = *it;

		if (_tmp_timeNs - entry->timeEnd() > _endOfTransmissionDelay) {

			if ((entry->timeEnd() - entry->timeStart()) < _minimumTransmissionLength) {
#if 0
				fprintf(stderr, "Invalid transmission terminated: ");
				entryToStderr(entry);
				fprintf(stderr, "\n");
#endif
			} else {
				trueDetection++;
#if 0
				fprintf(stderr, "Transmission terminated: ");
				entryToStderr(entry);
				fprintf(stderr, "\n");
#endif
			}
			totalDetections++;
			it = _activeComs.erase(it);
			delete entry;
		}
	}

	//fprintf(stderr, "false alarm ratio = %f\n", ((float)falseDetection) / totalDetections);

	this->_tmp_timeNs = 0;
}

bool RadioEventTable::toStringBufferReserve(size_t offset, size_t additional)
{
	if (_toStringBufSize - offset < additional) {
		_toStringBufSize *= 2; /* Yeah, I know, that may be over the top! */
		char * tmp = (char *) realloc(_toStringBuf, _toStringBufSize);
		if (tmp == NULL)
			return false;
		_toStringBuf = tmp;
	}

	return true;
}

bool RadioEventTable::addCommunicationToString(size_t &offset, RetEntry *entry)
{
	size_t len = _toStringBufSize - offset;
	if (!entry->toString(_toStringBuf + offset, &len))
		return false;
	offset += len;
	return true;
}

bool RadioEventTable::toString(char **buf, size_t *len)
{
	size_t offset = 0;
	char *_b = _toStringBuf;

	*buf = NULL;
	*len = 0;

	/* add the on-going communications */
	if (!toStringBufferReserve(offset, (1 + RetEntry::stringSize()) * _activeComs.size()))
		return false;

	std::list<RetEntry *>::iterator it;
	for (it = _activeComs.begin(); it != _activeComs.end(); ++it) {
		write_and_update_offset(offset, _b, (char) ACTIVE_COM);
		if (!addCommunicationToString(offset, (*it)))
			return false;
	}

	/* add finished communications */
	for (uint64_t i = _finishedComs.tail(); i < _finishedComs.head(); i++) {
		RetEntry *entry = _finishedComs.at_unsafe(i);

		if (entry->isDirty()) {
			write_and_update_offset(offset, _b, (char) FINISHED_COM);
			if (!addCommunicationToString(offset, (*it)))
				return false;
			entry->setDirty(false);
		}
	}

	*buf = _toStringBuf;
	*len = offset;

	return true;
}

bool RadioEventTable::updateFromString(const char *buf, size_t len)
{
	char comType = ACTIVE_COM;
	size_t offset = 0;

	while (offset < len && comType != PACKET_END) {
		read_and_update_offset(offset, buf, comType);
		if (comType < PACKET_END) {
			/* generate the entry */
			RetEntry *entry = new RetEntry();
			size_t entryLen = len - offset;
			if (!entry->fromString(buf+offset, &entryLen))
				return false;
			offset += entryLen;

			/* add it to the right table */
			if (comType == ACTIVE_COM)
				_activeComs.push_back(entry);
			else if (comType == ACTIVE_COM)
				_activeComs.push_back(entry);
		}
	}

	return true;
}
