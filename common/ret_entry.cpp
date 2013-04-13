#include "ret_entry.h"

#include "message_utils.h"

RetEntry::RetEntry(uint32_t id, uint64_t timeStart, uint64_t timeEnd,
			 uint32_t frequencyStart, uint32_t frequencyEnd,
			 int8_t pwr, Psu psu, uint64_t address):
	_id(id), _timeStart(timeStart), _timeEnd(timeEnd), _frequencyStart(frequencyStart), _frequencyEnd(frequencyEnd),
	_pwr(pwr), _dirty(true)
{	
	setAddress(address);
	setPsu(psu);
}

bool RetEntry::fromString(const char *buf, size_t length)
{
	if (length < stringSize())
		return false;

	size_t offset = 0;
	read_and_update_offset(offset, buf, _id);
	read_and_update_offset(offset, buf, _timeStart);
	read_and_update_offset(offset, buf, _timeEnd);
	read_and_update_offset(offset, buf, _frequencyStart);
	read_and_update_offset(offset, buf, _frequencyEnd);
	read_and_update_offset(offset, buf, _pwr);
	read_and_update_offset(offset, buf, _address);

	return true;
}

bool RetEntry::toString(char *buf, size_t &length)
{
	if (length < stringSize())
		return false;

	size_t offset = 0;
	write_and_update_offset(offset, buf, _id);
	write_and_update_offset(offset, buf, _timeStart);
	write_and_update_offset(offset, buf, _timeEnd);
	write_and_update_offset(offset, buf, _frequencyStart);
	write_and_update_offset(offset, buf, _frequencyEnd);
	write_and_update_offset(offset, buf, _pwr);
	write_and_update_offset(offset, buf, _address);

	return true;
}

RetEntry::Psu RetEntry::psu() const {
	return (Psu)(_address >> 56);
}

uint64_t RetEntry::address() const{
	return _address & 0xFFFFFFFFFFFFFF;
}

void RetEntry::setId(uint64_t id){
	_id = id;
}

void RetEntry::setTimeStart(uint64_t timeStart){
	_timeStart = timeStart;
}

void RetEntry::setTimeEnd(uint64_t timeEnd){
	_timeEnd = timeEnd;
}

void RetEntry::setFrequencyStart(uint32_t frequencyStart){
	_frequencyStart = frequencyStart;
}

void RetEntry::setFrequencyEnd(uint32_t frequencyEnd){
	_frequencyEnd = frequencyEnd;
}

void RetEntry::setPwr(int8_t pwr){
	_pwr = pwr;
}

void RetEntry::setPsu(Psu psu){
	_address &= 0x00FFFFFFFFFFFFFF;
	_address |= ((uint64_t)psu) << 56;
}

void RetEntry::setAddress(uint64_t address)
{
	_address &= 0xFF00000000000000;
	_address |= (address & 0xFFFFFFFFFFFFFF);
}
