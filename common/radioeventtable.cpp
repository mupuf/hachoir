#include "radioeventtable.h"

#include "message_utils.h"

bool RadioEventTable::fuzzyCompare(uint32_t a, uint32_t b, int32_t maxError)
{
	int32_t sub = a - b;

	bool ret = ((-sub) < maxError) && (sub < maxError);
	/*fprintf(stderr,
		"fuzzyCompare: a = %i, b = %i, sub = %i, maxError = %u, res = %s\n",
		a, b, sub, maxError, ret?"true":"false");*/
	return ret;
}

RetEntry * RadioEventTable::findMatchInActiveCommunications(uint32_t frequencyStart,
				      uint32_t frequencyEnd,
				      int8_t pwr)
{
	RetEntry * entry;

	uint32_t comWidth = frequencyEnd - frequencyStart;
	uint32_t comCentralFreq = frequencyStart + comWidth / 2;
	int32_t maxError = comWidth * 10 / 100;

	std::list< std::shared_ptr<RetEntry> >::iterator it;
	for (it = _activeComs.begin(); it != _activeComs.end(); ++it) {
		entry = (*it).get();

		uint32_t width = entry->frequencyEnd() - entry->frequencyStart();
		uint32_t centralFreq = entry->frequencyStart() + width / 2;

		if (/*fuzzyCompare(comWidth, width, maxError) &&*/
		    fuzzyCompare(comCentralFreq, centralFreq, maxError)) {
			return entry;
		}
	}

	/*if (frequencyStart == 936000)
		fprintf(stderr, "Couldn't match com [%u, %u], centralFreq = %u, width = %u\n",
		frequencyStart, frequencyEnd, comCentralFreq, comWidth);*/

	return NULL;
}

RadioEventTable::RadioEventTable(size_t ringSize, uint32_t endOfTransmissionDelay,
				 uint32_t minimumTransmissionLength) : _currentComID(0),
	_finishedComs(ringSize), _endOfTransmissionDelay(endOfTransmissionDelay),
	_minimumTransmissionLength(minimumTransmissionLength)
{
	trueDetection = totalDetections = 0;
	_toStringBufSize = 1000000; /* 1 MB */
	_toStringBuf = (char *)malloc(_toStringBufSize);

}

RadioEventTable::~RadioEventTable()
{
	free(_toStringBuf);
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


void RadioEventTable::addCommunication(uint64_t frequencyStart,
				       uint64_t frequencyEnd,
				       int8_t pwr)
{
	RetEntry * entry = findMatchInActiveCommunications(frequencyStart, frequencyEnd, pwr);
#if DEBUG_ADD_COMMUNICATION
	fprintf(stderr, "	%s a match for ", entry?"Found":"Didn't find");
	entryToStderr(entry);
	fprintf(stderr, " --> ");
#endif
	if (entry == NULL) {
		/* no corresponding entry found, let's create one! */
		entry = new RetEntry(_currentComID++, _tmp_timeNs, _tmp_timeNs, frequencyStart,
					    frequencyEnd, pwr, RetEntry::UNKNOWN, 0);
		_activeComs.push_back(std::shared_ptr<RetEntry>(entry));
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
			entry->setFrequencyEnd(frequencyEnd);
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
	std::list< std::shared_ptr<RetEntry> >::iterator it;
	for (it = _activeComs.begin(); it != _activeComs.end(); ++it) {
		entry = (*it).get();

		if (_tmp_timeNs - entry->timeEnd() > _endOfTransmissionDelay) {

			if ((entry->timeEnd() - entry->timeStart()) < _minimumTransmissionLength) {
#if 0
				fprintf(stderr, "Invalid transmission terminated: ");
				entryToStderr(entry);
				fprintf(stderr, "\n");
#endif
			} else {
				trueDetection++;
				_finishedComs.push_back((*it));
#if 0
				fprintf(stderr, "Transmission terminated: ");
				entryToStderr(entry);
				fprintf(stderr, "\n");
#endif
			}
			totalDetections++;
			it = _activeComs.erase(it);
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

	std::list< std::shared_ptr<RetEntry> >::iterator it;
	for (it = _activeComs.begin(); it != _activeComs.end(); ++it) {
		RetEntry * entry = (*it).get();
		if (entry->timeEnd() != entry->timeStart()) {
			write_and_update_offset(offset, _b, (char) ACTIVE_COM);
			if (!addCommunicationToString(offset, entry))
				return false;
		}
	}

	/* add finished communications */
	for (uint64_t i = _finishedComs.tail(); i < _finishedComs.head(); i++) {
		RetEntry *entry = _finishedComs.at_unsafe(i);

		if (entry->isDirty()) {
			write_and_update_offset(offset, _b, (char) FINISHED_COM);
			if (!addCommunicationToString(offset, entry))
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

	/* empty the active coms list */
	_activeComs.clear();

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
				_activeComs.push_back(std::shared_ptr<RetEntry>(entry));
			else if (comType == FINISHED_COM)
				_finishedComs.push_back(entry); /* this works right now because we never update old entries */
		}
	}

	return true;
}

std::vector< std::shared_ptr<RetEntry> >
RadioEventTable::fetchEntries(uint64_t timeStart, uint64_t timeEnd)
{
	std::vector< std::shared_ptr<RetEntry> > ret;

	/* copy all the active communications */
	std::copy(_activeComs.begin(), _activeComs.end(),
		  std::back_insert_iterator< std::vector< std::shared_ptr<RetEntry> > >(ret));

	/* now look at the finished communications */
	for (uint64_t i = _finishedComs.tail(); i < _finishedComs.head(); i++) {
		RetEntry *entry = _finishedComs.at_unsafe(i);

		if (entry->timeEnd() > timeStart &&
		    entry->timeStart() < timeEnd)
		    ret.push_back(_finishedComs.at(i));
	}

	return ret;
}
