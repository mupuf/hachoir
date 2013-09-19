#ifndef COMSDETECT_H
#define COMSDETECT_H

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include "fft.h"
#include "calibrationpoint.h"
#include <vector>

class ComsDetect
{
	uint16_t _fftSize;
	uint32_t _comMinFreqWidth;
	uint32_t _comMinSNR;
	uint64_t _comMinDurationNs;
	uint64_t _comEndOfTransmissionDelay;
	uint16_t _maxTimeout;

	CalibrationPoint calib;

	CalibrationPoint *_calibs;

	struct lastDetectedTransmission
	{
		CalibrationPoint *calib;

		uint32_t inactiveCnt;
		uint32_t avgCnt;

		float avg;
		float avgSquared;
		bool active;
	} * _lastDetectedTransmission;

	float probaPwrUnder(float pwr);
	uint32_t calcInactiveTimeout(float pwr, float confidence);
	size_t calibPointIndexAt(size_t i) const;

	friend ComsDetect &comsDetect();
	ComsDetect(uint32_t comMinFreqWidth,
		uint32_t comMinSNR, uint64_t comMinDurationNs,
		uint64_t comEndOfTransmissionDelay);
public:

	void setFftSize(uint16_t fftSize);
	uint16_t fftSize() const { return _fftSize; }
	uint32_t comMinFreqWidth() const { return _comMinFreqWidth; }
	uint32_t comMinSNR() const { return _comMinSNR; }
	uint64_t comMinDurationNs() const { return _comMinDurationNs; }
	uint64_t comEndOfTransmissionDelay() const { return _comEndOfTransmissionDelay; }

	void addFFT(boost::shared_ptr<Fft> fft);

	bool isBinActive(size_t i) const { return _lastDetectedTransmission[i].active; }
	float noiseFloor(size_t i) const { return _lastDetectedTransmission[i].calib->modelMean(); }
	float noiseMax(size_t i) const { return _lastDetectedTransmission[i].calib->modelMax(); }
	float avgPowerAtBin(size_t i) const;
	float varianceAtBin(size_t i) const;
};

ComsDetect &comsDetect();

#endif // COMSDETECT_H
