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

#ifndef INCLUDED_GTSRC_HACHOIR_F_IMPL_H
#define INCLUDED_GTSRC_HACHOIR_F_IMPL_H

#include <gtsrc/hachoir_c.h>
#include <gr_firdes.h>

#include <stdint.h>
#include <memory>

#include <boost/array.hpp>
#include <boost/thread.hpp>

#include "radioeventtable.h"
#include "samplesringbuffer.h"
#include "sensingserver.h"
#include "fftwindow.h"
#include "fft.h"

namespace gr {
namespace gtsrc {

	class hachoir_c_impl : public hachoir_c
	{
	private:
		/* parameters */
		uint64_t _freq;
		uint64_t _samplerate;
		uint16_t _fft_size;
		gr_firdes::win_type _window_type;

		/* server */
		SensingServer _server;

		/* time-domain ring buffer */
		SamplesRingBuffer _ringBuf;

		/* Radio Event Table */
		RadioEventTable _ret;

		/* internals */
		boost::thread fftThread;
		FftWindow win;

		void sendFFT(const Fft *fft, const char *filteredFft);
		void sendRetUpdate();
		void calc_fft();
		void calcThermalNoise(const char *outputFile = NULL);

		void update_fft_params(int fft_size, gr_firdes::win_type window_type);
		uint64_t getTimeNs();

	public:
		hachoir_c_impl(double freq, int samplerate, int fft_size, int window_type);
		~hachoir_c_impl();

		void forecast (int noutput_items, gr_vector_int &ninput_items_required);

		// Where all the action really happens
		int general_work(int noutput_items,
				gr_vector_int &ninput_items,
				gr_vector_const_void_star &input_items,
				gr_vector_void_star &output_items);

		uint64_t central_freq() const { return _freq;}
		uint64_t sample_rate() const { return _samplerate;}
		uint16_t  fft_size() const { return _fft_size;}
		gr_firdes::win_type window_type() const { return _window_type;}

		void set_central_freq(double freq) { _freq = freq;}
		void set_sample_rate(int samplerate) { _samplerate = samplerate;}
		void set_FFT_size(int fft_size) { update_fft_params(fft_size, window_type()); }
		void set_window_type(int win_type) { update_fft_params(fft_size(), (gr_firdes::win_type) win_type); }
	};

} // namespace gtsrc
} // namespace gr

#endif /* INCLUDED_GTSRC_HACHOIR_F_IMPL_H */

