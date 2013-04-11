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
#include "fftaverage.h"

#include <stdio.h>
#include <iostream>

void ringBufferTest();

namespace gr {
namespace gtsrc {

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
			_freq(freq), _samplerate(samplerate),
			socket(ios), _ringBuf(10000 * fft_size)
	{
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 21334);
		socket.connect(endpoint);

		this->set_max_noutput_items(fft_size/4);

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

		//fprintf(stderr, "noutput_items = %u\n", noutput_items);

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

		uint64_t pos = _ringBuf.addSamples(in, noutput_items);

		RBMarker m = { central_freq(), sample_rate(), curTime };
		_ringBuf.addMarker(m, pos);

		_ringBuf.validateWrite();

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

	void
	hachoir_c_impl::calc_fft()
	{
		int id = 0;
		FftAverage avr(fft_size(), central_freq(), sample_rate(), 18);
		size_t comMinWidth = 8;
		char comMinSNR = 8;
		int period = 1;

		gri_fft_complex fft(fft_size());

		uint8_t buffer[5120];
		union {
			uint8_t  *u08;
			char     *s08;
			uint16_t *u16;
			uint32_t *u32;
			uint64_t *u64;
			float *f;
		} ptr;

		while (_ringBuf.size() < fft_size())
			fftThread.yield();

		uint64_t lastUpdate = getTimeNs();
		uint64_t lastFFtTime = 0, FFtTimeAverage = 0;
		uint64_t fftCount = 0;

		while (1)
		{
			ptr.u08 = buffer;

			boost::shared_ptr<Fft> new_fft(new Fft(fft_size(),
							       central_freq(),
							       sample_rate(),
							       &fft,
							       win, _ringBuf));


			if (lastFFtTime > 0)
				FFtTimeAverage += (new_fft->time_ns() - lastFFtTime);
			lastFFtTime = new_fft->time_ns();

			avr.addFft(new_fft);

			uint64_t curTime = getTimeNs();
			uint64_t time_diff = curTime - lastUpdate;
			if (time_diff > 1000000000) {
				float fftRate = fftCount / ((float)time_diff / 1000000000);
				float fftCoverage = fftRate * fft_size() / sample_rate();
				fprintf(stderr, "fftCoverage = %f, averageFftTime = %f: FFT rate = %f (fftCount = %llu, time_diff = %llu)\n",
					fftCoverage, ((float)FFtTimeAverage) / fftCount, fftRate, fftCount, time_diff);
				lastUpdate = curTime;
				fftCount = 0;
				FFtTimeAverage = 0;
			} else
				fftCount++;

#if 1
			if ((id++ % period) == 0) {
				float noiseFloor = avr.noiseFloor();

				*ptr.u08++ = 1;
				*ptr.u08++ = 0;
				*ptr.u16++ = avr.fftSize(); //steps
				*ptr.u64++ = avr.startFrequency(); // start freq
				*ptr.u64++ = avr.endFrequency(); // end freq
				*ptr.u64++ = avr.time_ns();

				char *pwrs = ptr.s08;
				for (int i = 0; i < avr.fftSize(); i++) {
					char pwr = (char) (avr[i] - noiseFloor);
					if (pwr < comMinSNR)
						pwr = 0;
					*ptr.s08++ = pwr;

				}

				size_t comWidth = 0;
				for (int i = 0; i < avr.fftSize(); i++) {
					if (pwrs[i] > 0)
						comWidth++;
					else {
						if (comWidth < comMinWidth) {
							for (int e = i - comWidth; e < i; e++)
								pwrs[e] = 0;
						} else {
							/* add the communication to the radio event table */
						}

						comWidth = 0;
					}
				}

				socket.send(boost::asio::buffer(buffer, ptr.u08 - buffer));
			}
#endif

		}
	}

} /* namespace gtsrc */
} /* namespace gr */

#if 0
void ringBufferTest()
{
	RingBuffer<float, float> r(10);

	r.debugContent();

	float a[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 };
	r.addSamples(a, 23);
	r.debugContent();
	r.validateWrite();
	r.debugContent();

	r.clear();

	r.debugContent();
	for (float i = 0; i < 20; i++) {
		r.addSamples(&i, 1);
		if (((int)i % 3) == 0)
			r.addMarker(i, i);
		r.validateWrite();
		r.debugContent();
	}

	for (float i = 0; i < 30; i++) {
		float *sample;
		size_t len = 5, newLen = len;
		bool found = r.requestRead(i, &newLen, &sample);
		fprintf(stderr, "requestRead(%.0f, %i): ", i, len);
		fprintf(stderr, "ret = %s", found?"true":"false");
		fprintf(stderr, ", len = %i", newLen);
		if (found) {
			fprintf(stderr, ", sample = ( ", sample[0]);
			for (int e = 0; e < newLen; e++)
				fprintf(stderr, "%.0f ", sample[e]);
			fprintf(stderr, ")");
		}
		fprintf(stderr, "\n");
	}

	for (float i = 0; i < 30; i++) {
		float marker = 0;
		bool found = r.getMarker(i, &marker);
		fprintf(stderr, "getMarker(%.0f): found = %s, marker = %.0f\n",
		i, found?"true":"false", marker);
	}

	for (float i = 0; i < 30; i++) {
		float marker;
		uint64_t markerPos;
		marker = markerPos = 0;
		bool found = r.findMarker(i, &marker, &markerPos);
		fprintf(stderr, "findMarker(%.0f): found = %s, marker = %.0f, markerPos = %llu\n",
		i, found?"true":"false", marker, markerPos);
	}

	exit(1);
}
#endif
