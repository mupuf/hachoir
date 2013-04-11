/**
 * \file      fft.h
 * \author    MuPuF (Martin Peres <martin.peres@labri.fr>)
 * \version   1.0
 * \date      08 Avril 2013
 */

#ifndef FFT_H
#define FFT_H

#include <stdint.h>
#include <sys/time.h>

#include <gri_fft.h>

#include "fftwindow.h"
#include "samplesringbuffer.h"

/**
 * \class     Fft
 * \brief     Generates a Fast Fourier Transform (FFT).
 *
 * \details   When given a stream of sample taken at sampleRate, the FFT will
 *            translate it into the frequency domain which will span from
 *            -sampleRate/2 to sampleRate/2.
 *
 *            The FFT can be made of any size power-of-two size lower or equal
 *            to 8192. The frequency resolution depends on the FFT Size with the
 *            following relation: frequencyResolution = sampleRate/FFTSize.
 *
 *            An FFT needs as many samples as its size to get an accurate power
 *            reading, which means that for doing an FFT 512, 512 samples are
 *            needed. If you don't have enough samples, the FFT class will add
 *            trailing 0 to allow you to get a good frequency resolution at the
 *            expense of a low power resolution.
 *
 *            The output of the FFT is stored as dBM.
 *
 *            **Thread-safety:** Once constructed, the object should be thread safe.
 */
class Fft
{
protected:
	uint16_t _fft_size; ///< The size of the FFT, no shit Sherlock!
	uint64_t _central_frequency; ///< The central frequency at which the samples were taken
	uint64_t _sample_rate; ///< The samples' sampling rate
	uint64_t _time_ns; ///< The time in ns at which the first sample has been received (relative to 01/01/1970)

	std::vector<float> _pwr; ///< The buffer that stores the FFT bins, converted to dBM

	/**
	 * \brief    Generate the FFT
	 *
	 * \details  Before calling this function, please apply the fftWindow
	 *           \a win
	 *
	 * \param    fftSize   The size of the FFT
	 * \param    win       The window that has been applied to the sample stream
	 * \param    fft       A gri_fft_complex that will perform the FFT
	 * \return   Nothing.
	 */
	void doFFt(uint16_t fftSize, FftWindow &win, gri_fft_complex *fft);

	Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate);
public:
	/**
	 * \brief    Create the FFT from a generic sample source.
	 *
	 * \details  See the class's description for more details
	 *
	 * \param  fftSize           The size of the FFT
	 * \param  centralFrequency  The central frequency at which the samples were taken
	 * \param  sampleRate        The samples' sampling rate
	 * \param  fft               A gri_fft_complex that will perform the FFT
	 * \param  win               The window to apply on the samples
	 * \param  src               The source samples to perform the FFT on
	 * \param  length            The length of the source (should be <= FFT Size)
	 * \param  time_ns           The time in ns at which the first sample has been received (relative to 01/01/1970)
	 * \return Nothing.
	 */
	Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate,
	    gri_fft_complex *fft, FftWindow &win, const gr_complex *src, size_t length,
	    uint64_t time_ns);

	/**
	 * \brief    Create the FFT from a SampleRingBuffer.
	 *
	 * \details  See the class's description for more details
	 *
	 * \param  fftSize           The size of the FFT
	 * \param  centralFrequency  The central frequency at which the samples were taken
	 * \param  sampleRate        The samples' sampling rate
	 * \param  fft               A gri_fft_complex that will perform the FFT
	 * \param  win               The window to apply on the samples
	 * \param  ringBuffer        The samples' source. The latest n latest samples will be used to calculate the FFT
	 * \return Nothing.
	 */
	Fft(uint16_t fftSize, uint64_t centralFrequency, uint64_t sampleRate,
	    gri_fft_complex *fft, FftWindow &win, SamplesRingBuffer &ringBuffer);


	uint16_t fftSize() const { return _fft_size; }
	uint64_t centralFrequency() const { return _central_frequency; }
	uint64_t sampleRate() const { return _sample_rate; }
	virtual uint64_t time_ns() const { return _time_ns; }

	/**
	 * \brief    Get the lowest frequency represented by this FFT.
	 *
	 * \return   The frequency associated with bin/index 0.
	 */
	uint64_t startFrequency() const { return freqAtBin(0); }

	/**
	 * \brief    Get the highest frequency represented by this FFT.
	 *
	 * \return   The frequency associated with bin/index (fftSize - 1).
	 */
	uint64_t endFrequency() const { return freqAtBin(_fft_size - 1); }

	/**
	 * \brief    Get the frequency associated with bin/index \a i.
	 *
	 * \param    i    The bin number.
	 * \return   The frequency associated with bin/index \a i.
	 */
	uint64_t freqAtBin(uint16_t i) const
	{
		assert(i < _fft_size);
		return _central_frequency - (_sample_rate / 2) + i * _sample_rate / _fft_size;
	}

	/// Returns the noise floor in dBM
	virtual float noiseFloor() const;

	/**
	 * \brief    Get the power at bin \a i
	 *
	 * \param    i    The bin number.
	 * \return   The power associated with bin/index \a i.
	 */
	virtual float operator[](size_t i) const { return _pwr[i]; }

	virtual Fft & operator+=(const Fft &other); ///< Add two FFTs
	virtual Fft &  operator-=(const Fft &other); ///< Substract two FFTs
	virtual const Fft operator+(const Fft &other) const; ///< Add two FFTs
	virtual const Fft operator-(const Fft &other) const; ///< Substract two FFTs
};

#endif // FFT_H
