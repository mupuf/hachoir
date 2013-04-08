/**
 * \file      fftwindow.h
 * \author    MuPuF (Martin Peres <martin.peres@labri.fr>)
 * \version   1.0
 * \date      08 Avril 2013
 */

#ifndef FFTWINDOW_H
#define FFTWINDOW_H

#include <gr_firdes.h>
#include <vector>
#include <stdint.h>

/**
 * \class     FftWindow
 * \brief     Generates FFT window
 *
 * \details   Look at https://en.wikipedia.org/wiki/Window_function.
 *
 * **Thread-safety:** Once constructed, the object should be thread safe.
 */
class FftWindow
{
private:
	uint16_t _fft_size; ///< The size of the FFT, no shit Sherlock!
	gr_firdes::win_type _window_type; ///< The GNU Radio type for the window
	float _window_power; ///< The power associated with the window

	std::vector<float> win; ///< The window

public:
	FftWindow();

	/**
	 * \brief    Generate an FftWindow.
	 *
	 * \details  Before calling this function, please apply the fftWindow
	 *           \a win
	 *
	 * \param    fftSize      The size of the FFT
	 * \param    window_type  The type of window to be generated (WIN_HAMMING = 0,
	 *                        WIN_HANN = 1, WIN_BLACKMAN = 2, WIN_RECTANGULAR = 3,
	 *                        WIN_KAISER = 4, WIN_BLACKMAN_HARRIS = 5)
	 *
	 * \return   Nothing.
	 */
	FftWindow(uint16_t fftSize, gr_firdes::win_type window_type);

	/// reset the FFT Window (equivalent to a square window)
	void reset(uint16_t fftSize, gr_firdes::win_type window_type);

	uint16_t fftSize() const { return _fft_size; }
	gr_firdes::win_type windowType() const { return _window_type; }
	float windowPower() const { return _window_power; }

	/**
	 * \brief    Get the power at bin \a i
	 *
	 * \param    i    The bin number.
	 * \return   The power associated with bin/index \a i.
	 */
	float operator[](size_t i) const { return win.at(i); }
};

#endif // FFTWINDOW_H
