#include "fftwindow.h"

FftWindow::FftWindow()
{

}

FftWindow::FftWindow(uint16_t fftSize, gr::filter::firdes::win_type window_type)
{
	reset(fftSize, window_type);
}

void FftWindow::reset(uint16_t fftSize, gr::filter::firdes::win_type window_type)
{
	win = gr::filter::firdes::window(window_type, fftSize, 6.76); // 6.76 is cargo-culted

	_window_power = 0;
	for (int i = 0; i < fftSize; i++)
		_window_power += win[i]*win[i];
}
