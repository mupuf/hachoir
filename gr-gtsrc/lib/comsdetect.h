#ifndef COMSDETECT_H
#define COMSDETECT_H

#include <stdint.h>

class ComsDetect
{
	uint32_t _fftAverageCount;
	uint32_t _comMinFreqWidth;
	uint32_t _comMinSNR;
	uint64_t _comMinDurationNs;
	uint64_t _comEndOfTransmissionDelay;
	float _noiseFloor;

	friend ComsDetect &comsDetect();

	ComsDetect(uint32_t fftAverageCount, uint32_t comMinFreqWidth,
		uint32_t comMinSNR, uint64_t comMinDurationNs,
		uint64_t comEndOfTransmissionDelay,
		float _noiseFloor);
public:

	uint32_t fftAverageCount() const { return _fftAverageCount; }
	uint32_t comMinFreqWidth() const { return _comMinFreqWidth; }
	uint32_t comMinSNR() const { return _comMinSNR; }
	uint64_t comMinDurationNs() const { return _comMinDurationNs; }
	uint64_t comEndOfTransmissionDelay() const { return _comEndOfTransmissionDelay; }

	float noiseFloor() { return _noiseFloor; }
};

ComsDetect &comsDetect();

#endif // COMSDETECT_H
