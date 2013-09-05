#include "comsdetect.h"
#include <assert.h>
#include <string.h>
#include <math.h>

#define CALIBRATION_POINT_COUNT 5

float ComsDetect::probaPwrUnder(float pwr)
{
	float mu = 1.67;
	float sigma = 4.5578;
	return 1 - expf(-expf(-(-pwr + mu) / sigma));
}

uint32_t ComsDetect::calcInactiveTimeout(float pwr, float confidence)
{
	float p = probaPwrUnder(pwr);
	float n = logf(1 - confidence) / logf(p);

	if (n > 100.0)
		return 100;
	else
		return ceil(n);
}

ComsDetect &comsDetect()
{
	static ComsDetect detect(500000, 10, 100000000, 1000000);
	return detect;
}

ComsDetect::ComsDetect(uint32_t comMinFreqWidth,
    uint32_t comMinSNR, uint64_t comMinDurationNs,
    uint64_t comEndOfTransmissionDelay) :
	_comMinFreqWidth(comMinFreqWidth), _comMinSNR(comMinSNR),
	_comMinDurationNs(comMinDurationNs),
	_comEndOfTransmissionDelay(comEndOfTransmissionDelay),
	_lastDetectedTransmission(nullptr)
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

	/* assign every calibrationValue */
	for (uint16_t i = 0; i < fftSize; i++) {
		int offset = i * CALIBRATION_POINT_COUNT / fftSize;

		_lastDetectedTransmission[i].calib =
				&_calibs[offset];

#if 0
		// debug density
		std::ostringstream filename;
		filename << "/tmp/density_" << offset << ".csv";
		_lastDetectedTransmission[i].calib->dumpToCsvFile(filename.str());
#endif
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

	for (int i = 0; i < fftSize(); i++) {
		float pwr = (fft->operator [](i));
		struct lastDetectedTransmission *lt = &_lastDetectedTransmission[i];

		lt->calib->addData(fft->operator [](i));
		if (!lt->calib->isReady())
			continue;

		if (pwr > lt->calib->modelMax()) {
			lt->inactiveCnt = 0;
			lt->active = true;
		}

		if (lt->active) {
			lt->avg += pwr;
			lt->avgCnt++;
			if (lt->avgCnt > 1000) {
				lt->avgCnt /= 2;
				lt->avg /= 2;
			}

			if (pwr < comMinSNR()) {
				lt->inactiveCnt++;
				float avg = lt->avg / lt->avgCnt;
				uint32_t timeout = calcInactiveTimeout(lt->calib->modelMax() - avg, 0.9999);
				if (lt->inactiveCnt >= timeout) {
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
	if (lt->avgCnt > 0) {
		float nf = noiseFloor(i);
		float avg = lt->avg / lt->avgCnt;
		return avg < nf ? nf : avg;
	} else
		return noiseFloor(i);
}
