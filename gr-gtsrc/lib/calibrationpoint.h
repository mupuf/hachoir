#ifndef CALIBRATIONPOINT_H
#define CALIBRATIONPOINT_H

#include <stdint.h>
#include <string>

class CalibrationPoint
{
	uint32_t _maxSampleCount;
	uint32_t _minSampleCount;
	uint32_t _lastUpdate;

	uint32_t _distrib[256];
	uint32_t _samplesCount;

	float _realMean, _realMax;
	float _modelMean, _modelMax;

	std::string _dumpFile;

	uint8_t pwrToIndex(float power) const;
	float indexToPwr(float index) const;
	void calcStats();

public:
	CalibrationPoint();
	CalibrationPoint(uint32_t maxSampleCount, uint32_t minSampleCount);
	void addData(float power);
	void reset();

	void dumpToCsvFile(const std::string &filepath);

	float realMean() const { return _realMean; }
	float realMax() const { return _realMax; }
	float modelMean() const { return _modelMean; }
	float modelMax() const { return _modelMax; }

	bool isReady() const { return _samplesCount >= _minSampleCount; }
	uint32_t samplesCount() { return _samplesCount; }

	uint32_t maxSampleCount() const { return _maxSampleCount; }
	void setMaxSampleCount(uint32_t maxSampleCount);

	uint32_t updatePeriod() const { return _minSampleCount; }
	void setMinSampleCount(uint32_t minSampleCount);
};

#endif // CALIBRATIONPOINT_H
