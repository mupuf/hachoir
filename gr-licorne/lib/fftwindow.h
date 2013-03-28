#ifndef FFTWINDOW_H
#define FFTWINDOW_H

#include <gr_firdes.h>
#include <vector>
#include <stdint.h>

class FftWindow
{
private:
	uint16_t _fft_size;
	gr_firdes::win_type _window_type;
	float _window_power;

	std::vector<float> win;

public:
	FftWindow();
	FftWindow(uint16_t fft_size, gr_firdes::win_type window_type);

	void reset(uint16_t fft_size, gr_firdes::win_type window_type);

	uint16_t fftSize() const { return _fft_size; }
	gr_firdes::win_type windowType() const { return _window_type; }
	float windowPower() const { return _window_power; }

	float operator[](size_t index) const { return win.at(index); }
};

#endif // FFTWINDOW_H
