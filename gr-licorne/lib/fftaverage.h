#ifndef FFTAVERAGE_H
#define FFTAVERAGE_H

#include "fft.h"
#include <deque>
#include <boost/shared_ptr.hpp>

class FftAverage : public Fft
{
private:
	std::deque< boost::shared_ptr<Fft> > ffts;

	size_t _average;

public:
	FftAverage(uint16_t fftSize, uint64_t centralFrequency,
		   uint64_t sampleRate, size_t average);

	void addFft(boost::shared_ptr<Fft> fft);
	void reset();

	size_t averageCount() const { return _average; }
	size_t currentAverageCount() const { return ffts.size(); }
	uint64_t time_ns() const { return ffts.at(0)->time_ns(); }

	virtual float operator[](size_t i) const { return _pwr[i] / currentAverageCount(); }
};

#endif // FFTAVERAGE_H
