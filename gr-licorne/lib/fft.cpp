#include "fft.h"

Fft::Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate,
	 gri_fft_complex *fft, FftWindow &win, const gr_complex *src, size_t length,
	 uint64_t time_ns) : _fft_size(fftSize),
	_central_frequency(centralFrequency), _sample_rate(sampleRate),
	_time_ns(time_ns), pwr(fftSize)
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

	fft->execute();

	/* process the output */
	gr_complex *out = fft->get_outbuf();
	for (i = 0; i < fftSize; i++) {
		int n = (i + fftSize/2 ) % fftSize;

		float real = out[n].real();
		float imag = out[n].imag();
		float mag = sqrt(real*real + imag*imag);

		pwr[i] = 20 * log10(fabs(mag))
			- 20 * log10(fftSize)
			- 10 * log10(win.windowPower()/fftSize)
			+ 3;
	}
}
