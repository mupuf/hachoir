#include "calibrationpoint.h"
#include <memory.h>
#include <sys/time.h>
#include <stdio.h>
#include <fstream>
#include "sw_radio_params.h"

uint64_t get_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_sec * 1000000 + tv.tv_usec;
}

uint8_t CalibrationPoint::pwrToIndex(float power) const
{
	int16_t index = (150.0 + power) * 2.0;

	if (index < 0)
		index = 0;
	else if (index > 255)
		index = 255;

	return index;
}

float CalibrationPoint::indexToPwr(float index) const
{
	return (index / 2.0) - 150.0;
}

void CalibrationPoint::calcStats()
{
	uint64_t weightedSum = 0;
	uint8_t maxPwr = 0, maxProb = 0;
	uint32_t maxProbVal = 0;

	int maxPwrLimit = pwrToIndex(-60.0);
	for (int i = 0; i < 256; i++) {
		weightedSum += (i * _distrib[i]);
		if (_distrib[i] > 0)
			maxPwr = i;
		if (i <= maxPwrLimit && _distrib[i] > maxProbVal) {
			maxProbVal = _distrib[i];
			maxProb = i;
		}
	}

	_realMean = indexToPwr((float) weightedSum / (float) _samplesCount);
	_realMax = indexToPwr(maxPwr);

	_modelMean = indexToPwr(maxProb) - NOISE_MU;
	_modelMax = _modelMean + 10 / _samplesCount + 14;

	if (_dumpFile != std::string()) {
		std::ofstream myfile;

		myfile.open (_dumpFile);

		myfile << "power, p" << std::endl;

		for (int i = 0; i < 256; i++)
			myfile << indexToPwr(i) << ", " << _distrib[i] << std::endl;

		myfile << ",,Real mean, " << realMean() << std::endl;
		myfile << ",,Real max, " << realMax() << std::endl;

		myfile << ",,Model (kind of) mean, " << modelMean() << std::endl;
		myfile << ",,Model max, " << modelMax() << std::endl;

		myfile.close();
	}
}

CalibrationPoint::CalibrationPoint() :
	_maxSampleCount(10000000),
	_minSampleCount(100)
{
	reset();
}

CalibrationPoint::CalibrationPoint(uint32_t maxSampleCount, uint32_t minSampleCount):
	_maxSampleCount(maxSampleCount), _minSampleCount(minSampleCount)
{
	reset();
}

void CalibrationPoint::addData(float power)
{
	/* we store power with a 0.5dB accuracy, ranging from -150dBm to -22dBm */
	int16_t index = pwrToIndex(power);

	_distrib[index]++;
	_samplesCount++;

	/* make sure the sample count never exceeds _maxSampleCount */
	if (_samplesCount >= _maxSampleCount) {
		calcStats();

		for (uint16_t i = 0; i < sizeof(_distrib) / sizeof(uint32_t); i++) {
			_distrib[i] /= 2;
		}
		_samplesCount /= 2;
	} else if (_samplesCount >= _lastUpdate * 10 &&
		   _samplesCount >= _minSampleCount) {
		calcStats();
		_lastUpdate = _samplesCount;
	}
}

void CalibrationPoint::reset()
{
	_samplesCount = 0;
	_lastUpdate = 0;
	memset(_distrib, 0, sizeof(this->_distrib));
}

void CalibrationPoint::dumpToCsvFile(const std::string &filepath)
{
	_dumpFile = filepath;
}

void CalibrationPoint::setMaxSampleCount(uint32_t maxSampleCount)
{
	this->_maxSampleCount = maxSampleCount;
}

void CalibrationPoint::setMinSampleCount(uint32_t minSampleCount)
{
	this->_minSampleCount = minSampleCount;
}
