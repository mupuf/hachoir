#ifndef COMSDETECT_H
#define COMSDETECT_H

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include "fft.h"

class ComsDetect
{
	uint16_t _fftSize;
	uint32_t _fftAverageCount;
	uint32_t _comMinFreqWidth;
	uint32_t _comMinSNR;
	uint64_t _comMinDurationNs;
	uint64_t _comEndOfTransmissionDelay;
	float _noiseFloor;

	struct lastDetectedTransmission
	{
		uint64_t time;
		uint32_t inactiveCnt;
		uint32_t avgCnt;
		float avg;
		bool active;
	} * _lastDetectedTransmission;

	friend ComsDetect &comsDetect();

	ComsDetect( uint32_t fftAverageCount, uint32_t comMinFreqWidth,
		uint32_t comMinSNR, uint64_t comMinDurationNs,
		uint64_t comEndOfTransmissionDelay,
		float _noiseFloor);
public:

	void setFftSize(uint16_t fftSize);
	uint16_t fftSize() const { return _fftSize; }
	uint32_t fftAverageCount() const { return _fftAverageCount; }
	uint32_t comMinFreqWidth() const { return _comMinFreqWidth; }
	uint32_t comMinSNR() const { return _comMinSNR; }
	uint64_t comMinDurationNs() const { return _comMinDurationNs; }
	uint64_t comEndOfTransmissionDelay() const { return _comEndOfTransmissionDelay; }

	float noiseFloor() { return _noiseFloor; }

	void addFFT(boost::shared_ptr<Fft> fft);

	bool isBinActive(size_t i) const { return _lastDetectedTransmission[i].active; }
	float avgPowerAtBin(size_t i) const;
};

ComsDetect &comsDetect();

#endif // COMSDETECT_H
