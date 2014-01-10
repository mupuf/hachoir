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


#include "qa_hachoir_c.h"

#include <cppunit/TestAssert.h>
#include <gtsrc/hachoir_c.h>
#include <gnuradio/top_block.h>
#include <gnuradio/blocks/file_source.h>
#include <gnuradio/blocks/throttle.h>

#include <stdlib.h>
#include <string>

namespace gr {
namespace gtsrc {

	void
	qa_hachoir_c::t1()
	{
		gr::top_block_sptr topblock = gr::make_top_block("test_hachoir");
		int samplerate;

		std::string filepath = std::string(getenv("HOME")) + "/gsm_940.samples";
		samplerate = 8000000;
		hachoir_c::sptr hachoir = hachoir_c::make(940000000, samplerate, 1024, 1);

		/*std::string filepath = std::string(getenv("HOME")) + "/gsm_938_1.samples";
		samplerate = 1000000;
		hachoir_c::sptr hachoir = hachoir_c::make(938000000, samplerate, 1024, 1);*/

		gr::blocks::file_source::sptr file = gr::blocks::file_source::make(sizeof(gr_complex), filepath.c_str(), true);
		gr::blocks::throttle::sptr throttle = gr::blocks::throttle::make(sizeof(gr_complex), samplerate);
		topblock->connect(file, 0, throttle, 0);
		topblock->connect(throttle, 0, hachoir, 0);
		topblock->run();
	}

} /* namespace gtsrc */
} /* namespace gr */

