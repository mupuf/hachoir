#!/usr/bin/env python
# 
# Copyright 2013 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr, gr_unittest,analog,uhd
import gtsrc_swig as gtsrc

from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import uhd
from gnuradio import window
from gnuradio.eng_option import eng_option
from gnuradio.gr import firdes
from gnuradio.wxgui import fftsink2
from gnuradio.wxgui import forms
from gnuradio.wxgui import scopesink2
from gnuradio.wxgui import waterfallsink2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import numpy
import threading
import time
import wx
import os

class qa_hachoir_c (gr_unittest.TestCase):

	def setUp (self):
		self.tb = gr.top_block ()

	def tearDown (self):
		self.tb = None

	def test_002_t (self):
		samp_rate = 8000000
		freq=0.940e9
		#freq=2.464e9
		sqr = gtsrc.hachoir_c(freq=freq, samplerate=samp_rate, fft_size=256, window_type=1)
		filepath=os.getenv("HOME") + "/gsm_940.samples"
		self.gr_file_source_0 = gr.file_source(gr.sizeof_gr_complex*1, filepath, True)
		self.tb.connect((self.gr_file_source_0, 0), (sqr, 0))
		self.tb.run ()
		# check data

	def test_001_t(self):
		samp_rate = 2000000
		freq=0.7745e9
		#freq=2.464e9
		gain=60
		ant = "TX/RX"
		ant = "RX2"
		self.uhd_usrp_source_0 = uhd.usrp_source(
					device_addr="",
					stream_args=uhd.stream_args(
						cpu_format="fc32",
						channels=range(1),
					),
				)
		self.uhd_usrp_source_0.set_time_source("external", 0)
		self.uhd_usrp_source_0.set_subdev_spec("A:0", 0)
		#self.uhd_usrp_source_0.set_subdev_spec("B:0", 0)
		self.uhd_usrp_source_0.set_samp_rate(samp_rate)
		self.uhd_usrp_source_0.set_center_freq(freq, 0)
		self.uhd_usrp_source_0.set_gain(gain, 0)
		self.uhd_usrp_source_0.set_antenna(ant, 0)
		self.uhd_usrp_source_0.set_bandwidth(samp_rate, 0)
		sqr = gtsrc.hachoir_c(freq=freq, samplerate=samp_rate, fft_size=1024, window_type=1)
		self.tb.connect((self.uhd_usrp_source_0, 0), (sqr, 0))
		self.tb.run ()
		# check data

if __name__ == '__main__':
	gr_unittest.run(qa_hachoir_c, "qa_hachoir_c.xml")
