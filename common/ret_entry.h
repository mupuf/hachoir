#ifndef RE_ENTRY_H
#define RE_ENTRY_H

#include<iostream>
#include<string.h>
#include<vector>
#include"stdint.h"

class RetEntry
{
	uint64_t _id;
	uint64_t _timeStart;
	uint64_t _timeEnd;
	uint32_t _frequencyStart;
	uint32_t _frequencyEnd;
	int8_t _pwr;
	bool _dirty;

	//first 2 bits are for Psu
	uint64_t _address;

public:
	enum Psu { UNKNOWN = 0, PRIMARY = 1, SECONDARY = 2 };

	RetEntry() {}

	/// address is up to 56 bits
	RetEntry(uint32_t id, uint64_t timeStart, uint64_t timeEnd,
	      uint32_t frequencyStart, uint32_t frequencyEnd,
	      int8_t pwr, Psu psu, uint64_t address);

	static size_t stringSize() { return 41; }

	bool fromString(const char *buf, size_t length);
	bool toString(char *buf, size_t &length);

	uint64_t id() const { return _id;}
	uint64_t timeStart() const { return _timeStart;}
	uint64_t timeEnd() const { return _timeEnd;}
	uint32_t frequencyStart() const { return _frequencyStart;}
	uint32_t frequencyEnd() const { return _frequencyEnd;}
	int8_t pwr() const { return _id;}
	Psu psu() const;

	bool isDirty() const { return _dirty; }

	/// address is up to 56 bits
	uint64_t address() const;

	void setId(uint64_t id);
	void setTimeStart(uint64_t timeStart);
	void setTimeEnd(uint64_t timeEnd);
	void setFrequencyStart(uint32_t frequencyStart);
	void setFrequencyEnd(uint32_t frequencyEnd);
	void setPwr(int8_t pwr);
	void setPsu(Psu psu);
	void setDirty(bool value) { _dirty = value; }

	/// address is up to 56 bits
	void setAddress(uint64_t address);
};

#endif // ENTRY_H
