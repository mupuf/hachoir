#ifndef HOPPINGPATTERN_H
#define HOPPINGPATTERN_H

#include "band.h"
#include <vector>
#include <stdint.h>
#include <cstddef>

class HoppingPattern
{
public:
	enum SensingType { LINEAR = 0, RANDOM = 1 };
	enum OperationMode { AVAILABLE, SENSE };

private:
	struct entry_t {
		Band b;
		float start;
		float len;
	};

	std::vector<entry_t> _hops;
	Band _tunBand;
	float _spectrumWindow;
	SensingType _sensingType;
	uint64_t _hoppingPeriod;

	uint64_t _period;
	uint64_t _curOffset;

	float _sensingFreq;
public:
	HoppingPattern();
	HoppingPattern(uint64_t period_us, Band tunBand, float spectrumWindow,
			SensingType sensingType, uint64_t hoppingPeriod);

	void addAvailableEntry(Band b, float start, float len);

	void setPeriod_us(uint64_t period) { _period = period; }
	uint64_t period_us() const { return _period; }

	size_t bandsCount() const { return _hops.size(); }
	Band bandAt(size_t idx) const;
	size_t beaconSize() const { return 15 + _hops.size() * 10; }

	void addTicks(uint64_t us);

	bool bandInfo(size_t idx, uint64_t &activeSince_us, uint64_t &activeFor_us) const;
	bool isBandActive(size_t idx) const;
	bool isBandIncludedInCurrentBands(Band b) const;
	bool isSensing(uint64_t &sensingSince_us,
		       uint64_t &sensingFor_us,
		       uint64_t &sensingNextJump) const;
};

#endif // HOPPINGPATTERN_H
