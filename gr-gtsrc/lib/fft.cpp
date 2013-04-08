#include "fft.h"

#include <iostream>

void Fft::doFFt(uint16_t fftSize, FftWindow &win, gri_fft_complex *fft)
{
	int i;

	fft->execute();

	/* process the output */
	gr_complex *out = fft->get_outbuf();
	float logFftSize = log10(fftSize);
	float logWindowPower = log10(win.windowPower()/fftSize);
	for (i = 0; i < fftSize; i++) {
		int n = (i + fftSize/2 ) % fftSize;

		float real = out[n].real();
		float imag = out[n].imag();
		float mag = sqrt(real*real + imag*imag);

		_pwr[i] = 20 * (float)log10(fabs(mag))
			- 20 * logFftSize
			- 10 * logWindowPower
			+ 3;
	}
}

Fft::Fft()
{

}

Fft::Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate,
	 gri_fft_complex *fft, FftWindow &win, const gr_complex *src, size_t length,
	 uint64_t time_ns) : _fft_size(fftSize),
	_central_frequency(centralFrequency), _sample_rate(sampleRate),
	_time_ns(time_ns), _pwr(fftSize)
{
	int i;

	gr_complex *dst = fft->get_inbuf();

	/* apply the window and copy to input buffer */
	for (i = 0; i < length && i < fftSize; i++) {
		dst[i] = src[i] * win[i];
	}

	/* add padding samples when length < fft_size */
	for (i = i; i < fftSize; i++)
		dst[i] = 0;

	doFFt(fftSize, win, fft);
}

Fft::Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate,
    gri_fft_complex *fft, FftWindow &win, SamplesRingBuffer &ringBuffer)
	: _fft_size(fftSize), _central_frequency(centralFrequency),
	  _sample_rate(sampleRate), _pwr(fftSize)
{
	gr_complex *dst = fft->get_inbuf();
	gr_complex *samples;

	size_t startPos, restartPos;
	size_t length = ringBuffer.requestReadLastN(fftSize, &startPos, &restartPos, &samples);

	_time_ns = ringBuffer.timeAt(startPos);

	size_t currentPos = 0;
	do
	{
		int i;

		/* apply the window and copy to the input buffer */
		for (i = 0; i < length; i++) {
			dst[i] = samples[i] * win[i];
		}
		currentPos += length;

		if (currentPos < fftSize) {
			size_t len = fftSize - currentPos;
			bool found = ringBuffer.requestRead(restartPos, &len, &samples);
		}
	} while(currentPos < fftSize);

	/* check that the data has not been overriden while we were reading it! */
	if (!ringBuffer.isPositionValid(startPos))
		return;

	doFFt(fftSize, win, fft);
}

Fft & Fft::operator+=(const Fft &rhs)
{
	/* check that we are adding an fft of the same type */
	if (rhs.fftSize() != fftSize() ||
	    rhs.centralFrequency() != centralFrequency() ||
	    rhs.sampleRate() != sampleRate()) {
		std::cerr << "FftAverage::operator+: Tried to add an fft with different characteristics";
		std::cerr << std::endl;
		return *this;
	}

	for (int i = 0; i < fftSize(); i++)
		_pwr[i] += rhs.operator [](i);

	return *this;
}

Fft & Fft::operator-=(const Fft &rhs)
{
	std::cerr << "operator-=(): start" << std::endl;

	/* check that we are adding an fft of the same type */
	if (rhs.fftSize() != fftSize() ||
	    rhs.centralFrequency() != centralFrequency() ||
	    rhs.sampleRate() != sampleRate()) {
		std::cerr << "FftAverage::operator+: Tried to add an fft with different characteristics";
		std::cerr << std::endl;
		return *this;
	}

	std::cerr << "operator-=(): 2" << std::endl;

	for (int i = 0; i < fftSize(); i++)
		_pwr[i] -= rhs.operator [](i);

	std::cerr << "operator-=(): end" << std::endl;
	return *this;
}

const Fft Fft::operator+(const Fft &other) const
{
	return Fft(*this) += other;
}

const Fft Fft::operator-(const Fft &other) const
{
	return Fft(*this) -= other;
}
