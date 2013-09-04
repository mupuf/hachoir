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

#include "comsdetect.h"
#include "radioeventtable.h"
#include "../common/message_utils.h"

void ringBufferTest();

namespace gr {
namespace gtsrc {

	hachoir_c::sptr
	hachoir_c::make(double freq, int samplerate, int fft_size, int window_type)
	{
		try {
			return gnuradio::get_initial_sptr (new hachoir_c_impl(freq, samplerate, fft_size, window_type));
		}
		catch (std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			return hachoir_c::sptr();
		}
	}
	/*
	* The private constructor
	*/
	hachoir_c_impl::hachoir_c_impl(double freq, int samplerate, int fft_size, int window_type)
		: gr_block("hachoir_f",
			gr_make_io_signature(1, 1, sizeof (gr_complex)),
			gr_make_io_signature(0, 0, sizeof (gr_complex))),
			_freq(freq), _samplerate(samplerate),
			_server(21333), _ringBuf(1000 * fft_size),
			_ret(1000, comsDetect().comEndOfTransmissionDelay(), comsDetect().comMinDurationNs())
	{
		comsDetect().setFftSize(fft_size);

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

	void hachoir_c_impl::sendFFT(const Fft *fft, const char *filteredFft)
	{
		uint8_t packet[4200];

		union {
			uint8_t  *u08;
			char     *s08;
			uint16_t *u16;
			uint32_t *u32;
			uint64_t *u64;
			float *f;
		} ptr;

		ptr.u08 = packet;
		*ptr.u08++ = (char) MSG_FFT;
		*ptr.u32++ = 26 +  fft->fftSize();
		*ptr.u16++ = fft->fftSize(); //steps
		*ptr.u64++ = fft->startFrequency(); // start freq
		*ptr.u64++ = fft->endFrequency(); // end freq
		*ptr.u64++ = fft->time_ns();

		for (int i = 0; i < fft->fftSize(); i++)
			*ptr.u08++ = (char) filteredFft[i]; // (fft->operator [](i));

		_server.sendToAll((char*)packet, ptr.u08 - packet);
	}

	void hachoir_c_impl::sendRetUpdate()
	{
		char msgHeader[5];
		size_t offset = 0;

		char * buf;
		size_t len;
		_ret.toString(&buf, &len);

		/* generate the message Header */
		write_and_update_offset(offset, msgHeader, (char) MSG_RET);
		write_and_update_offset(offset, msgHeader, len);
		_server.sendToAll(msgHeader, 5);

		/* send the actual payload */
		_server.sendToAll(buf, len);

		//fprintf(stderr, "len = %u\n", len);
	}

	void
	hachoir_c_impl::calc_fft()
	{
		char filteredFFT[4096];

		/* for statistics */
		uint64_t lastUpdate = getTimeNs();
		uint64_t lastFFtTime = 0, FFtTimeAverage = 0;
		uint64_t fftCount = 0;

		/* parameters for the detection */
		int id = 0;
		size_t comMinWidth = 4;

		/* the fft calculator */
		gri_fft_complex fft(fft_size());

		uint64_t start_pos = _ringBuf.tail();
		while (1)
		{
		/* calculating the FFT */
			boost::shared_ptr<Fft> new_fft(new Fft(fft_size(),
							       central_freq(),
							       sample_rate(),
							       &fft,
							       win, _ringBuf,
							       start_pos));
			start_pos = new_fft->ringBufferStartPos() + fft_size();




		/* detecting transmissions */
			comsDetect().addFFT(new_fft);

			for (int i = 0; i < fft_size(); i++) {
				float pwr = comsDetect().avgPowerAtBin(i);
				filteredFFT[i] = pwr;
			}

#if 0
			_ret.startAddingCommunications(new_fft->time_ns());
			uint16_t comWidth = 0;
			int32_t sumPwr = 0;
			for (int i = 0; i < fft_size(); i++) {
				if (filteredFFT[i] > 0) {
					sumPwr += filteredFFT[i];
					comWidth++;
				} else {
					if (comWidth < comMinWidth) {
						for (int e = i - comWidth; e < i; e++)
							filteredFFT[e] = 0;
					} else {
						int8_t avgPwr = comsDetect().noiseFloor(i) + (int8_t)(sumPwr / comWidth);

						/* add the communication to the radio event table */
						_ret.addCommunication(new_fft->freqAtBin(i - comWidth)/1000,
								      new_fft->freqAtBin(i - 1)/1000,
								      avgPwr);
					}

					sumPwr = 0;
					comWidth = 0;
				}
			}
			_ret.stopAddingCommunications();
#endif

		/* some stats, sorry about this code */
			if (lastFFtTime > 0)
				FFtTimeAverage += (new_fft->time_ns() - lastFFtTime);
			lastFFtTime = new_fft->time_ns();

			uint64_t curTime = getTimeNs();
			uint64_t time_diff = curTime - lastUpdate;
			if (time_diff > 1000000000) {
				float fftRate = fftCount / ((float)time_diff / 1000000000);
				float fftCoverage = fftRate * fft_size() / sample_rate();
				fprintf(stderr, "fftCoverage = %f, averageFftTime = %f: FFT rate = %f (fftCount = %llu, time_diff = %llu), detections = %llu/%llu\n",
					fftCoverage, ((float)FFtTimeAverage) / fftCount, fftRate, fftCount, time_diff, _ret.trueDetection, _ret.totalDetections);
				lastUpdate = curTime;
				fftCount = 0;
				FFtTimeAverage = 0;
				_ret.trueDetection = 0;
			} else
				fftCount++;


		/* send the Ffts to the clients! */
			if ((id++ % 10) == 0) {
				sendFFT(new_fft.get(), filteredFFT);
				sendRetUpdate();
				_server.matchActiveCommunications(_ret);
			}

		}
	}

	void hachoir_c_impl::calcThermalNoise(const char *outputFile)
	{
		gri_fft_complex fft(fft_size());
		FftAverage avr(fft_size(), central_freq(), sample_rate(), 500000); //500000
		size_t profile[2000];
		size_t profile_length = 2000;
		float profile_steps = 0.1;

		/* init the profile */
		for (size_t i = 0; i < profile_length; i++)
			profile[i] = 0;

		/* calculate the FFTs and add this into the average FFT */
		uint64_t pos = _ringBuf.tail();
		size_t length;
		do {
			gr_complex *samples = nullptr;
			length = fft_size();

			_ringBuf.requestRead(pos, &length, &samples);
			if (length == fft_size()) {
				boost::shared_ptr<Fft> new_fft(new Fft(fft_size(),
								       central_freq(),
								       sample_rate(),
								       &fft, win,
								       samples, length, 0));
				avr.addFft(new_fft);
				pos += length; //length; //go for precision and not for computation time;
			}

		} while (length == fft_size() && avr.currentAverageCount() < avr.averageCount());

		/* calculate the variance at each bin and add data to the profile */
		float bin_variance_average = 0;
		float* variance = new float[fft_size()];
		for (size_t i = 0; i < fft_size(); i++) {
			float bin_avr;
			variance[i] = avr.varianceAt(i, &bin_avr, profile, profile_length, profile_steps);

			bin_variance_average += variance[i];

			/*fprintf(stderr, "bin %i: [%f, %f, %f]\n",
				i, bin_avr, variance[i], sqrtf(variance[i]));*/
		}

		/* sum the number of occurences found at each point of the profile */
		uint64_t profileCountSum = 0;
		for (size_t i = 0; i < profile_length; i++)
			profileCountSum += profile[i];

		/* get the probability distribution of the noise */
		fprintf(stderr, "Profile:\n");
		double probaSum;
		for (size_t i = 0; i < profile_length; i++) {
			float offset = ((float)i - (profile_length / 2)) * profile_steps;
			probaSum += ((double)profile[i]) / profileCountSum;

			/*if (probaSum > 0 && probaSum < 1)
				fprintf(stdout, "%.3f, %.20f\n", offset, probaSum);*/
			fprintf(stdout, "%.1f, %u\n", offset, profile[i]);
		}

		float bin_variance_variance = 0;
		bin_variance_average /= fft_size();

		for (size_t i = 0; i < fft_size(); i++) {
			float diff = variance[i] - bin_variance_average;
			bin_variance_variance += diff * diff;
		}
		bin_variance_variance /= fft_size();

		fprintf(stderr, "Variance of the noise variance: [%f, %f, %f]\n",
			bin_variance_average, bin_variance_variance,
			sqrtf(bin_variance_variance));

		fprintf(stderr, "Time span = %llu ns\n", avr.span_ns());
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
