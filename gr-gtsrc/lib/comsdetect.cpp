#include "comsdetect.h"

ComsDetect &comsDetect()
{
	static ComsDetect detect(4, 500000, 15, 500000, 1000000, -60.0);
	return detect;
}

ComsDetect::ComsDetect(uint32_t fftAverageCount, uint32_t comMinFreqWidth,
	uint32_t comMinSNR, uint64_t comMinDurationNs,
	uint64_t comEndOfTransmissionDelay, float _noiseFloor) :
	_fftAverageCount(fftAverageCount),
	_comMinFreqWidth(comMinFreqWidth), _comMinSNR(comMinSNR),
	_comMinDurationNs(comMinDurationNs),
	_comEndOfTransmissionDelay(comEndOfTransmissionDelay),
	_noiseFloor(noiseFloor())
{

}
