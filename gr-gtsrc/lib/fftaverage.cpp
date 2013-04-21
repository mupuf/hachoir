#include "fftaverage.h"

#include <iostream>
#include <algorithm>

FftAverage::FftAverage(uint16_t fftSize, uint64_t centralFrequency,
		       uint64_t sampleRate, size_t average) :
	Fft(fftSize, centralFrequency, sampleRate), _average(average)
{
	reset();
}

void FftAverage::addFft(boost::shared_ptr<Fft> fft)
{
	/* check that we are adding an fft of the same type */
	if (fft->fftSize() != fftSize() ||
	    fft->centralFrequency() != centralFrequency() ||
	    fft->sampleRate() != sampleRate()) {
		std::cerr << "FftAverage::addFft: Tried to add an fft with different characteristics";
		std::cerr << std::endl;
		return;
	}

	/* delete the oldest entry if the average buffer is full */
	if (ffts.size() == _average) {
		boost::shared_ptr<Fft> oldFft = ffts.at(0);

		for (size_t i = 0; i < fftSize(); i++)
			_pwr[i] -= oldFft->operator [](i);
		ffts.pop_front();
	}

	/* add the new fft */
	for (size_t i = 0; i < fftSize(); i++) {
		_pwr[i] += fft->operator [](i);
	}
	ffts.push_back(fft);
}

void FftAverage::reset()
{
	ffts.clear();
	for (int i = 0; i < fftSize(); i++)
		_pwr[i] = 0.0;
}

float FftAverage::noiseFloor() const
{
	return Fft::noiseFloor() / currentAverageCount();
}

float FftAverage::varianceAt(size_t i, float *avr, size_t *profile,
			     int profile_length)
{
	float average = operator [](i);
	if (avr)
		*avr = average;

	float variance = 0;
	for (size_t e = 0; e < ffts.size(); e++) {
		float v = ffts.at(e)->operator[](i);
		float diff = v - average;
		float val = diff * diff;
		variance += val;

		if (profile) {
			float pro_diff = v - average;
			if (pro_diff > (profile_length / 2))
				pro_diff = (profile_length / 2) - 1;
			else if (pro_diff < -(profile_length / 2))
				pro_diff = -(profile_length / 2);

			pro_diff = pro_diff + (profile_length / 2);
			int offset = (int) pro_diff;
			profile[offset]++;
		}
	}

	return variance / ffts.size();
}
