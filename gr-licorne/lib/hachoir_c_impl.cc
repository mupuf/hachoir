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

#include "fftaverage.h"

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
			_freq(freq), _samplerate(samplerate),
			_ringBuf(10000 * fft_size, samplerate)
	{
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 21334);
		socket.connect(endpoint);

		update_fft_params(fft_size, (gr_firdes::win_type) window_type);

		fftThread = boost::thread(&hachoir_c_impl::calc_fft, this);
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
		static uint64_t lastUpdate = 0;
		static uint64_t sampleCount = 0;
		const gr_complex *in = (const gr_complex *) input_items[0];
		int i;

		uint64_t curTime = getTimeNs();
		if (lastUpdate == 0)
			lastUpdate = curTime;
		else {
			uint64_t time_diff = curTime - lastUpdate;
			if (time_diff > 1000000000) {
				float sampleRate = sampleCount / ((float)time_diff / 1000000000);
				fprintf(stderr, "Sample rate = %f (sampleCount = %llu, time_diff = %llu)\n",
					sampleRate, sampleCount, time_diff);
				lastUpdate = curTime;
				sampleCount = 0;
			}
		}
		sampleCount += noutput_items;

		_ringBuf.addSamples(in, noutput_items);

		RBMarker m = { central_freq(), curTime };
		_ringBuf.addMarker(m, noutput_items);

		sleep(1);

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

		_fft_size = fft_size;
		_window_type = _window_type;
	}

	uint64_t
	hachoir_c_impl::getTimeNs()
	{
		struct timeval time;
		gettimeofday(&time, NULL);
		return (time.tv_sec * 1000000 + time.tv_usec) * 1000;
	}
	
	boost::shared_ptr<Fft>
	hachoir_c_impl::calc_fft()
	{
		int id = 0;
		FftAverage noisefloor(fft_size(), central_freq(), sample_rate(), 10000);
		FftAverage avr(fft_size(), central_freq(), sample_rate(), 10);
		int period = 3;

		gri_fft_complex fft(fft_size());

		uint8_t buffer[5120];
		union {
			uint8_t  *u08;
			uint16_t *u16;
			uint32_t *u32;
			uint64_t *u64;
			float *f;
		} ptr;

		while (_ringBuf.currentSize() < fft_size())
			fftThread.yield();

		uint64_t lastUpdate = getTimeNs();
		float lastFFtTime = 0, FFtTimeAverage = 0;
		uint64_t fftCount = 0;

		while (1)
		{
			ptr.u08 = buffer;

			uint64_t time_ns = getTimeNs();

			boost::shared_ptr<Fft> new_fft(new Fft(fft_size(),
							       central_freq(),
							       sample_rate(),
							       &fft,
							       win, _ringBuf));


			if (lastFFtTime > 0)
				FFtTimeAverage += (new_fft->time_ns() - lastFFtTime);
			else
				fprintf(stderr, "new_fft->time_ns() == 0!\n");
			lastFFtTime = new_fft->time_ns();

			noisefloor.addFft(new_fft);
			avr.addFft(new_fft);

			uint64_t curTime = getTimeNs();
			uint64_t time_diff = curTime - lastUpdate;
			if (time_diff > 1000000000) {
				float fftRate = fftCount / ((float)time_diff / 1000000000);
				float fftCoverage = fftRate * fft_size() / sample_rate();
				fprintf(stderr, "fftCoverage = %f, averageFftTime = %f: FFT rate = %f (fftCount = %llu, time_diff = %llu)\n",
					fftCoverage, FFtTimeAverage / fftCount, fftRate, fftCount, time_diff);
				lastUpdate = curTime;
				fftCount = 0;
				FFtTimeAverage = 0;
			} else
				fftCount++;

			if ((id++ % period) == 0) {
				/*Fft avr(avr);
				avr -= noisefloor;*/

				//std::cerr << averageFFTTime / period << std::endl;

				*ptr.u08++ = 1;
				*ptr.u08++ = 0;
				*ptr.u16++ = avr.fftSize(); //steps
				*ptr.u64++ = avr.startFrequency(); // start freq
				*ptr.u64++ = avr.endFrequency(); // end freq
				*ptr.u64++ = avr.time_ns();

				for (int i = 0; i < avr.fftSize(); i++) {
					*ptr.u08++ = (char) (avr[i]/* - noisefloor[i]*/);
				}

				socket.send(boost::asio::buffer(buffer, ptr.u08 - buffer));
			}
		}

		//return new_fft;
	}

} /* namespace licorne */
} /* namespace gr */

