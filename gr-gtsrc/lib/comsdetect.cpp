#include "comsdetect.h"
#include <assert.h>
#include <string.h>

#define CALIBRATION_POINT_COUNT 32

ComsDetect &comsDetect()
{
    static ComsDetect detect(500000, 10, 100000000, 1000000, -65.0);
	return detect;
}

ComsDetect::ComsDetect(uint32_t comMinFreqWidth,
    uint32_t comMinSNR, uint64_t comMinDurationNs,
    uint64_t comEndOfTransmissionDelay, float noiseFloor) :
	_comMinFreqWidth(comMinFreqWidth), _comMinSNR(comMinSNR),
	_comMinDurationNs(comMinDurationNs),
	_comEndOfTransmissionDelay(comEndOfTransmissionDelay),
	_noiseFloor(noiseFloor), _lastDetectedTransmission(nullptr)
{

}

void ComsDetect::setFftSize(uint16_t fftSize)
{
	if (_lastDetectedTransmission != nullptr)
		delete[] _lastDetectedTransmission;
	if (_calibs != nullptr) {
		delete[] _calibs;
	}

	_fftSize = fftSize;

	_lastDetectedTransmission = new struct lastDetectedTransmission[_fftSize];
	assert(_lastDetectedTransmission != nullptr);
	memset(_lastDetectedTransmission, 0,
	       _fftSize * sizeof(struct lastDetectedTransmission));

	_calibs = new CalibrationPoint[CALIBRATION_POINT_COUNT];
	assert(_calibs != nullptr);
	memset(_calibs, 0, CALIBRATION_POINT_COUNT * sizeof(CalibrationPoint));

	/* assign every calibrationValue */
	for (uint16_t i = 0; i < fftSize; i++) {
		_lastDetectedTransmission[i].calib =
				&_calibs[i / fftSize / CALIBRATION_POINT_COUNT];
	}
}

uint64_t getTimeNs()
{
	struct timeval time;
	gettimeofday(&time, NULL);
	return (time.tv_sec * 1000000 + time.tv_usec) * 1000;
}

void ComsDetect::addFFT(boost::shared_ptr<Fft> fft)
{
	if (fft->fftSize() != fftSize())
		return;

	uint64_t curTime = getTimeNs();
	/* TODO: Do some better optimisation based on the noise profile and probabilities */
	for (int i = 0; i < fftSize(); i++) {
		float pwr = (fft->operator [](i) - noiseFloor());
		struct lastDetectedTransmission *lt = &_lastDetectedTransmission[i];

		if (pwr >= comMinSNR()) {
			lt->time = getTimeNs(); //fft->time_ns();
			lt->inactiveCnt = 0;
			lt->active = true;
		}

		if (lt->active) {
			lt->avg += pwr;
			lt->avgCnt++;
			if (lt->avgCnt > 100) {
				lt->avgCnt /= 2;
				lt->avg /= 2;
			}

			if (pwr < comMinSNR()) {
				lt->inactiveCnt++;
				uint64_t timeDiff = (curTime - lt->time);
				if (/*lt->inactiveCnt > 10 &&*/
				    timeDiff > _comMinDurationNs) {
					//printf("inactiveCnt = %u, timeDiff = %u\n", lt->inactiveCnt, timeDiff);
					lt->avg = 0;
					lt->avgCnt = 0;
					lt->active = false;
				}
			}
		}
	}
}

float ComsDetect::avgPowerAtBin(size_t i) const
{
	struct lastDetectedTransmission *lt = &_lastDetectedTransmission[i];
	if (lt->avgCnt > 0)
		return lt->avg / lt->avgCnt;
	else
		return 0.0;
}
