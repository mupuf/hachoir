#include "fftaverage.h"

#include <iostream>

FftAverage::FftAverage(uint16_t fftSize, uint64_t centralFrequency,
		       uint64_t sampleRate, size_t average) :
	_average(average)
{
	this->_fft_size = fftSize;
	this->_central_frequency = centralFrequency;
	this->_sample_rate = sampleRate;
	this->_pwr.reserve(fftSize);

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

		for (int i = 0; i < fftSize(); i++)
			_pwr[i] -= oldFft->operator [](i);
		ffts.pop_front();
	}

	/* add the new fft */
	for (int i = 0; i < fftSize(); i++) {
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
