#include "fftwindow.h"

FftWindow::FftWindow()
{

}

FftWindow::FftWindow(uint16_t fft_size, gr_firdes::win_type window_type)
{
	reset(fft_size, window_type);
}

void FftWindow::reset(uint16_t fft_size, gr_firdes::win_type window_type)
{
	win = gr_firdes::window(window_type, fft_size, 6.76); // 6.76 is cargo-culted

	_window_power = 0;
	for (int i = 0; i < fft_size; i++)
		_window_power += win[i]*win[i];
}
