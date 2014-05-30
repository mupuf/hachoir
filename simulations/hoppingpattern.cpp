#include "hoppingpattern.h"

#include "random.h"

#include <iostream>

HoppingPattern::HoppingPattern() : _period(-1)
{

}

HoppingPattern::HoppingPattern(uint64_t period_us, Band tunBand,
			      float spectrumWindow, SensingType sensingType,
			      uint64_t hoppingPeriod) :
_tunBand(tunBand), _spectrumWindow(spectrumWindow), _sensingType(sensingType),
 _hoppingPeriod(hoppingPeriod), _period(period_us), _curOffset(0.0), _sensingFreq(-1)
{

}

void HoppingPattern::addAvailableEntry(Band b, float start, float len)
{
	entry_t e = {b, start, len};
	_hops.push_back(e);
}

Band HoppingPattern::bandAt(size_t idx) const
{
	if (idx < bandsCount())
		return _hops[idx].b;
	else
		return Band();
}

void HoppingPattern::addTicks(uint64_t us)
{
	if (period_us() < 0) {
		std::cerr << "HoppingPattern::addTicks: Cannot add ticks to "
			     "a hopping pattern without period!" << std::endl;
		return;
	}

	_curOffset += us;
	_curOffset %= period_us();

	// Check if we are in a sensing period
	float latest = 0.0;
	for (size_t i = 0; i < bandsCount(); i++) {
		if (isBandActive(i))
			return;

		if ((_hops[i].start + _hops[i].len) > latest)
			latest = (_hops[i].start + _hops[i].len);
	}
	uint64_t sensingSince_us = _curOffset - (latest * period_us());

	if (sensingSince_us % _hoppingPeriod)
		return;


	switch(_sensingType) {
	case LINEAR:
		_sensingFreq += _spectrumWindow;
		if (_sensingFreq < _tunBand.start() || _sensingFreq > _tunBand.end())
			_sensingFreq = _tunBand.start() + _spectrumWindow / 2;
		break;
	case RANDOM:
		float start = _tunBand.start() + _spectrumWindow / 2;
		float end = _tunBand.end() - _spectrumWindow / 2;
		uint64_t range = end - start;
		_sensingFreq = start + rand_cmwc() % range;
		break;
	}
	//std::cout << "Sensing Freq = " << _sensingFreq / 1e6 << std::endl;
}

bool HoppingPattern::bandInfo(size_t idx, uint64_t &activeSince_us, uint64_t &activeFor_us) const
{
	if (!isBandActive(idx)) {
		activeSince_us = -1;
		activeFor_us = period_us() + (_hops[idx].start * period_us()) - _curOffset;
		activeFor_us %= period_us();
		return false;
	}
	activeSince_us = _curOffset - (_hops[idx].start * period_us());
	activeFor_us = (_hops[idx].start + _hops[idx].len) * period_us() - _curOffset;

	return true;
}

bool HoppingPattern::isBandActive(size_t idx) const
{
	if (idx < bandsCount()) {
		return _curOffset >= (_hops[idx].start * period_us()) &&
		       _curOffset < (uint64_t)((_hops[idx].start + _hops[idx].len) * period_us());
	} else
		return false;
}

bool HoppingPattern::isSensing(uint64_t &sensingSince_us,
			       uint64_t &sensingFor_us,
			       uint64_t &sensingNextJump) const
{
	float latest = 0.0, next = 1.0;

	for (size_t i = 0; i < bandsCount(); i++) {
		if (isBandActive(i)) {
			sensingSince_us = -1;
			sensingFor_us = -1;
			return false;
		}

		if ((_hops[i].start + _hops[i].len) > latest)
			latest = (_hops[i].start + _hops[i].len);
		if (_hops[i].start < next)
			next = _hops[i].start;
	}

	sensingSince_us = _curOffset - (latest * period_us());
	sensingFor_us = next * period_us() - _curOffset;
	sensingNextJump = _hoppingPeriod - (sensingSince_us % _hoppingPeriod);

	return true;
}

bool HoppingPattern::isBandIncludedInCurrentBands(Band b) const
{
	uint64_t s1, s2, s3;

	if (isSensing(s1, s2, s3)) {
		return b.start() > _sensingFreq - _spectrumWindow / 2 &&
			b.end() < _sensingFreq + _spectrumWindow / 2;
	} else {
		for (size_t i = 0; i < _hops.size(); i++) {
			if(isBandActive(i) && _hops[i].b.isBandIncluded(b))
				return true;
		}
	}

	return false;
}
