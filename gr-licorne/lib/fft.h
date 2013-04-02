#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include <sys/time.h>

#include <gri_fft.h>

#include "fftwindow.h"
#include "samplesringbuffer.h"

class Fft
{
protected:
	uint16_t _fft_size;
	uint64_t _central_frequency;
	uint64_t _sample_rate;
	uint64_t _time_ns;

	std::vector<float> _pwr;

	void doFFt(uint16_t fftSize, FftWindow &win, gri_fft_complex *fft);
	Fft();
public:
	//Fft(const Fft &other);
	Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate,
	    gri_fft_complex *fft, FftWindow &win, const gr_complex *src, size_t length,
	    uint64_t time_ns);
	Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate,
	    gri_fft_complex *fft, FftWindow &win, SamplesRingBuffer &ringBuffer);

	uint64_t fftSize() const { return _fft_size; }
	uint64_t centralFrequency() const { return _central_frequency; }
	uint64_t sampleRate() const { return _sample_rate; }
	uint64_t time_ns() const { return _time_ns; }

	uint64_t startFrequency() const { return freqAtBin(0); }
	uint64_t endFrequency() const { return freqAtBin(_fft_size - 1); }

	uint64_t freqAtBin(uint16_t i) const
	{
		assert(i < _fft_size);
		return _central_frequency - (_sample_rate / 2) + i * _sample_rate / _fft_size;
	}
	virtual float operator[](size_t i) const { return _pwr[i]; }
	virtual Fft & operator+=(const Fft &other);
	virtual Fft &  operator-=(const Fft &other);
	virtual const Fft operator+(const Fft &other) const;
	virtual const Fft operator-(const Fft &other) const;
};

#endif // FFT_H
