/* -*- c++ -*- */
/* 
 * Copyright 2013 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_LICORNE_HACHOIR_F_IMPL_H
#define INCLUDED_LICORNE_HACHOIR_F_IMPL_H

#include <licorne/hachoir_c.h>
#include <gr_firdes.h>

#include <stdint.h>
#include <memory>

#include <boost/asio.hpp>
#include <boost/array.hpp>

#include "fftwindow.h"
#include "fft.h"

namespace gr {
namespace licorne {

	class hachoir_c_impl : public hachoir_c
	{
	private:
		/* parameters */
		double _freq;
		int _samplerate;
		int _fft_size;
		gr_firdes::win_type _window_type;

		/* sockets */
		boost::asio::io_service ios;
		boost::asio::ip::tcp::socket socket;

		/* internals */
		std::auto_ptr<gr_complex> _buffer;
		size_t _buffer_pos;
		FftWindow win;
		std::auto_ptr<gri_fft_complex> fft;
	
		Fft calc_fft(const gr_complex *src, size_t length);
		void update_fft_params(int fft_size, gr_firdes::win_type window_type);

	public:
		hachoir_c_impl(double freq, int samplerate, int fft_size, int window_type);
		~hachoir_c_impl();

		void forecast (int noutput_items, gr_vector_int &ninput_items_required);

		// Where all the action really happens
		int general_work(int noutput_items,
				gr_vector_int &ninput_items,
				gr_vector_const_void_star &input_items,
				gr_vector_void_star &output_items);
		
		double central_freq() const { return _freq;};
		int sample_rate() const { return _samplerate;};
		int fft_size() const { return _fft_size;};
		gr_firdes::win_type window_type() const { return _window_type;};
		
		void set_central_freq(double freq) { _freq = freq;};
		void set_sample_rate(int samplerate) { _samplerate = samplerate;};
		void set_FFT_size(int fft_size) { update_fft_params(fft_size, window_type()); };
		void set_window_type(int win_type) { update_fft_params(fft_size(), (gr_firdes::win_type) win_type); };
	};

} // namespace licorne
} // namespace gr

#endif /* INCLUDED_LICORNE_HACHOIR_F_IMPL_H */

