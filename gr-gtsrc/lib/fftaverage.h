/**
 * \file      fftaverage.h
 * \author    MuPuF (Martin Peres <martin.peres@labri.fr>)
 * \version   1.0
 * \date      08 Avril 2013
 */

#ifndef FFTAVERAGE_H
#define FFTAVERAGE_H

#include "fft.h"
#include <deque>
#include <boost/shared_ptr.hpp>

/**
 * \class     FftAverage
 * \brief     Create an FFT average sliding window of N FFTs.
 *
 * \details   All the operations are in O(1), yeah!
 *
 *            **Thread-safety:** Not thread safe.
 */
class FftAverage : public Fft
{
private:
	std::deque< boost::shared_ptr<Fft> > ffts; ///< Stores up to \a _average FFTs

	size_t _average; ///< The size of the FFT window

public:
	/**
	 * \brief    Create an FFT average sliding window.
	 *
	 * \details  See the class's description for more details
	 *
	 * \param  fftSize           The size of the FFT
	 * \param  centralFrequency  The central frequency at which the samples were taken
	 * \param  sampleRate        The samples' sampling rate
	 * \param  average           The size of the FFT window
	 * \return Nothing.
	 */
	FftAverage(uint16_t fftSize, uint64_t centralFrequency,
		   uint64_t sampleRate, size_t average);

	/**
	 * \brief    Add an FFT to the average sliding window.
	 *
	 * \details  This operation is done in O(1) and should be considered
	 *           copy-less.
	 *
	 * \param  fft           The FFT to be added.
	 * \return Nothing.
	 */
	void addFft(boost::shared_ptr<Fft> fft);

	/// Empty the window
	void reset();

	/// Returns the size of the FFT window
	size_t averageCount() const { return _average; }

	/// Returns the current number of FFT in the FFT window
	size_t currentAverageCount() const { return ffts.size(); }

	/// Returns the current noise floor
	virtual float noiseFloor() const;

	/// Returns the time of the oldest FFT in the FFT window
	uint64_t time_ns() const { return ffts.at(0)->time_ns(); }

	/// Returns the time difference between the last and the first FFT
	uint64_t span_ns() const { return ffts.at(ffts.size() - 1)->time_ns() - ffts.at(0)->time_ns(); }

	/// Calculates the variance at bin i
	float varianceAt(size_t i, float *avr = NULL, size_t *profile = NULL, int profile_length = 0, float steps = 0.1);

	float averageOverTime(int i, uint64_t timeNs) const;

	/// Access the average FFT's bins. See Fft::operator[].
	virtual float operator[](size_t i) const { return _pwr[i] / currentAverageCount(); }
};

#endif // FFTAVERAGE_H
