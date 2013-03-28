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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include <gri_fft.h>

#include "hachoir_c_impl.h"

#include <stdio.h>

namespace gr {
namespace licorne {

	hachoir_c::sptr
	hachoir_c::make(double freq, int samplerate, int fft_size, int window_type)
	{
		return gnuradio::get_initial_sptr (new hachoir_c_impl(freq, samplerate, fft_size, window_type));
	}

	/*
	* The private constructor
	*/
	hachoir_c_impl::hachoir_c_impl(double freq, int samplerate, int fft_size, int window_type)
	: gr_block("hachoir_f",
			gr_make_io_signature(1, 1, sizeof (gr_complex)),
			gr_make_io_signature(0, 0, sizeof (gr_complex))),
			socket(ios),
			_freq(freq), _samplerate(samplerate)
	{
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 21334);
		socket.connect(endpoint);

		update_fft_params(fft_size, (gr_firdes::win_type) window_type);
	}

	/*
	* Our virtual destructor.
	*/
	hachoir_c_impl::~hachoir_c_impl()
	{
	}

	void
	hachoir_c_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
	{
		ninput_items_required[0] = noutput_items;
	}

	int
	hachoir_c_impl::general_work (int noutput_items,
			gr_vector_int &ninput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
	{
		const gr_complex *in = (const gr_complex *) input_items[0];
		int i;
		
		i = 0;
		do {
			/* Fill up the buffer if possible */
			for (i = i; i < noutput_items && _buffer_pos < _fft_size; i++)
				_buffer.get()[_buffer_pos++] = in[i];
			
			/* Do the FFT */
			if (_buffer_pos == _fft_size)
			{
				calc_fft(_buffer.get(), _fft_size);
			}
			
			/* reset buffer_pos */
			_buffer_pos = 0;
		} while (i < noutput_items);
		
		consume_each (noutput_items);

		// Tell runtime system how many output items we produced.
		return noutput_items;
	}
	
	void 
	hachoir_c_impl::update_fft_params(int fft_size, gr_firdes::win_type window_type)
	{
		/* reconstruct the window when the fft_size of the window type changes */
		if (fft_size != _fft_size || window_type != _window_type)
			win.reset(fft_size, window_type);
		
		if (fft_size != _fft_size) {
			fft.reset(new gri_fft_complex (fft_size));
			_buffer.reset(new gr_complex[fft_size]);
			_buffer_pos = 0;
		}

		_fft_size = fft_size;
		_window_type = _window_type;
	}
	
	Fft
	hachoir_c_impl::calc_fft(const gr_complex *src, size_t length)
	{
		static int id = 0;
		int average = 1;
		struct timeval time;

		uint8_t buffer[5120];
		union {
			uint8_t  *u08;
			uint16_t *u16;
			uint32_t *u32;
			uint64_t *u64;
			float *f;
		} ptr;
		ptr.u08 = buffer;

		gettimeofday(&time, NULL);
		uint64_t time_ns = (time.tv_sec * 1000000 + time.tv_usec) * 1000;
		Fft new_fft(fft_size(), central_freq(), sample_rate(),
		     (gri_fft_complex *)fft.get(), win, src, length, time_ns);

		if ((id++ % 512) == 0) {
			*ptr.u08++ = 1;
			*ptr.u08++ = 0;
			*ptr.u16++ = new_fft.fftSize(); //steps
			*ptr.u64++ = new_fft.startFrequency(); // start freq
			*ptr.u64++ = new_fft.endFrequency(); // end freq
			*ptr.u64++ = new_fft.time_ns();

			for (int i = 0; i < new_fft.fftSize(); i++) {
				*ptr.u08++ = (char) (new_fft[i] / average);
			}

			socket.send(boost::asio::buffer(buffer, ptr.u08 - buffer));
		}

		//exit(1);
		
		return new_fft;
	}

} /* namespace licorne */
} /* namespace gr */

