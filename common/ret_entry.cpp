#include "ret_entry.h"

RetEntry::RetEntry(uint32_t id, uint64_t timeStart, uint64_t timeEnd,
			 uint32_t frequencyStart, uint32_t frequencyEnd,
			 int8_t pwr, Psu psu, uint64_t address):
	_id(id), _timeStart(timeStart), _timeEnd(timeEnd), _frequencyStart(frequencyStart), _frequencyEnd(frequencyEnd),
	_pwr(pwr), _dirty(true)
{	
	setAddress(address);
	setPsu(psu);
}

//TODO: extract psu bits
Psu RetEntry::psu() const {
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

