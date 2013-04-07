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
#include <licorne/hachoir_c.h>
#include <gr_top_block.h>
#include <gr_file_source.h>
#include <gr_throttle.h>

#include <stdlib.h>
#include <string>

namespace gr {
namespace licorne {

	void
	qa_hachoir_c::t1()
	{
		gr_top_block_sptr topblock = gr_make_top_block("test_hachoir");

		std::string filepath = std::string(getenv("HOME")) + "/gsm_940.samples";

		gr_file_source_sptr file = gr_make_file_source(sizeof(gr_complex), filepath.c_str(), true);
		gr_throttle::sptr throttle = gr_make_throttle(sizeof(gr_complex), 8000000);
		hachoir_c::sptr hachoir = hachoir_c::make(940000000, 8000000, 512, 1);
		topblock->connect(file, 0, throttle, 0);
		topblock->connect(throttle, 0, hachoir, 0);
		topblock->run();
	}

} /* namespace licorne */
} /* namespace gr */

