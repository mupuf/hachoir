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


#include "hachoir_f_impl.h"

#include <stdio.h>

namespace gr {
namespace licorne {

	hachoir_f::sptr
	hachoir_f::make(int freq, int samplerate, int fft_size, int window_type)
	{
		return gnuradio::get_initial_sptr (new hachoir_f_impl(freq, samplerate, fft_size, window_type));
	}

	/*
	* The private constructor
	*/
	hachoir_f_impl::hachoir_f_impl(int freq, int samplerate, int fft_size, int window_type)
	: gr_block("hachoir_f",
			gr_make_io_signature(1, 1, sizeof (float)),
			gr_make_io_signature(0, 0, sizeof (float))),
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
	hachoir_f_impl::~hachoir_f_impl()
	{
	}

	void
	hachoir_f_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
	{
		ninput_items_required[0] = noutput_items;
	}

	int
	hachoir_f_impl::general_work (int noutput_items,
			gr_vector_int &ninput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items)
	{
		const float *in = (const float *) input_items[0];
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
	hachoir_f_impl::update_fft_params(int fft_size, gr_firdes::win_type window_type)
	{
		/* reconstruct the window when the fft_size of the window type changes */
		if (fft_size != _fft_size || window_type != _window_type)
			win = gr_firdes::window(window_type, fft_size, 6.76); // 6.76 is cargo-culted
		
		if (fft_size != _fft_size) {
			fft.reset(new gri_fft_complex (fft_size));
			_buffer.reset(new float[fft_size]);
			_buffer_pos = 0;
		}

		_fft_size = fft_size;
		_window_type = _window_type;
	}
	
	std::vector<float>
	hachoir_f_impl::calc_fft(const float *src, size_t length)
	{
		static int id = 0;
		gr_complex *dst = fft->get_inbuf();
		size_t i;

		uint8_t buffer[5120];
		union {
			uint8_t  *u08;
			uint16_t *u16;
			uint32_t *u32;
			uint64_t *u64;
			float *f;
		} ptr;
		ptr.u08 = buffer;

		/* apply the window and copy to input buffer */
		for (i = 0; i < length && i < fft_size(); i++)
			dst[i] = src[i] * win[i];
		
		/* add padding samples when length < fft_size */
		for (i = i; i < fft_size(); i++)
			dst[i] = 0;
		
		/* execute the fft */
		fft->execute();
		
		/* process the output */
		std::vector<float> mag(length / 2);
		gr_complex *out = fft->get_outbuf();

		*ptr.u08++ = 1;
		*ptr.u64++ = 0; // central frequency
		*ptr.u64++ = sample_rate(); // sampling rate
		*ptr.u16++ = fft_size();
		for (i = 0; i < length / 2; i++) {
			float freq = i * sample_rate() / fft_size();
			float real = out[i].real();
			float imag = out[i].imag();
			mag[i] = sqrt(real*real + imag*imag);
			*ptr.u08++ = (char)mag[i];
			printf("%.0f,%f,%f\n", freq, mag[i], log(mag[i]));
		}
		printf("\n");

		socket.send(boost::asio::buffer(buffer, ptr.u08 - buffer));
		
		//exit(1);
		
		return mag;
	}

} /* namespace licorne */
} /* namespace gr */

