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

from gnuradio import gr, gr_unittest,analog
import licorne_swig as licorne

class qa_hachoir_f (gr_unittest.TestCase):

	def setUp (self):
		self.tb = gr.top_block ()

	def tearDown (self):
		self.tb = None

	#def test_001_t (self):
		#src_data = (
			#-0.84, -0.17,  0.19,  1.62, -0.18,  0.65, -0.55,  0.19,  0.93,  1.20,  1.53, -1.36,  0.05,  1.02, -1.23,  0.71, 
			#0.87, -0.79,  0.33,  0.21, -0.17, -0.32, -0.30,  0.52, -2.23,  0.26, -0.09, -1.65,  0.29,  0.30,  1.10,  0.74, 
			#-0.16, -0.60,  0.66, -0.03,  1.17, -1.05,  0.82, -1.41,  0.52, -0.73,  0.33, -0.04, -0.87,  0.77,  0.01, -0.89, 
			#-0.65, -0.01,  1.88, -2.48, -1.68, -0.76,  0.15,  0.22,  2.40,  1.91,  1.70,  0.81,  2.79,  0.03,  0.10,  1.78, 
			#0.30,  0.47,  0.19,  1.20,  1.75,  1.28, -0.95, -2.45,  1.73, -0.81, -0.46,  0.03, -0.77, -0.37,  0.91, -0.45, 
			#-1.15,  0.18,  1.75,  1.04,  1.23, -0.04, -1.79, -1.36, -0.09, -0.08,  0.63,  0.91, -1.26,  0.77, -0.42, -0.48, 
			#-0.54, -1.64,  0.78,  0.25, -0.38, -0.55, -0.22, -0.23, -2.21, -0.86, -0.35, -0.86, -2.33, -1.76,  0.07, -0.40, 
			#-1.13,  2.56,  2.53, -0.04,  0.65,  1.36, -1.02, -0.25,  0.85,  0.29,  1.42, -1.26, -1.02, -1.01,  0.55, -0.10, 
			#-0.85,  1.54, -0.11,  1.38,  1.96, -0.41,  1.43, -0.00,  0.99,  0.93, -0.22, -0.37,  1.76, -0.51,  1.82,  0.21, 
			#0.40,  0.80, -0.25,  1.67, -0.12,  0.82,  0.59, -0.53,  0.26, -0.11,  0.07, -1.42,  0.09,  0.98,  0.98, -0.24, 
			#0.60, -0.57,  0.29,  0.86,  0.27, -0.80,  0.62,  0.52, -0.58,  1.97, -0.61, -1.85,  0.26, -0.47, -0.01,  0.29, 
			#1.53,  0.95,  1.58, -0.19, -1.13, -0.27, -0.81,  1.53,  0.04,  0.05,  0.93,  0.52, -0.05, -0.03,  0.15, -0.66, 
			#-1.65, -0.59,  0.21,  2.73,  0.24, -2.27, -0.49, -1.35, -0.61, -0.87,  0.71, -0.64,  1.18,  0.35, -0.29, -0.35, 
			#-1.10,  1.16, -0.84, -0.03,  0.50, -0.71,  0.27, -1.45,  0.93, -1.00, -1.04,  0.87, -1.59, -0.45, -1.36, -0.53, 
			#-1.44,  1.06,  0.08, -1.15, -0.96,  1.45, -0.66,  0.21, -0.40, -0.59, -1.97, -0.26,  0.25, -2.30,  1.00,  0.54, 
			#0.49,  1.46,  0.26, -0.80, -0.92,  1.11,  1.20,  1.26,  2.03,  2.71,  0.95,  0.51, -1.66, -1.45, -0.00,  0.01, 
			#1.18, -1.08,  1.11,  2.83, -0.30, -1.74,  0.46,  0.36,  0.23,  0.04,  0.60,  1.43,  0.08,  0.65,  0.03, -0.09, 
			#0.59,  0.24,  1.78, -0.46, -0.58,  0.79,  0.68,  1.66,  0.42,  1.86,  0.11, -1.25,  0.40, -1.06, -0.32,  1.44, 
			#-0.17, -0.02,  0.29,  0.64, -0.78,  1.24, -2.19, -0.68,  2.11,  0.28,  0.17, -0.33,  0.50, -0.81,  0.43, -1.38, 
			#-1.41, -0.61, -0.30, -0.15,  0.38, -1.84,  1.12, -0.38, -0.68,  0.03,  0.18,  1.34, -1.67,  0.61, -0.52, -1.98, 
			#0.73,  0.79, -0.38, -0.34,  0.71, -0.01,  0.24, -0.55, -1.08, -0.03, -1.15,  0.38, -0.10,  0.00,  0.45, -0.04, 
			#0.71,  0.50, -0.50,  1.83, -0.71,  1.24, -0.37, -0.27, -1.70, -0.16, -1.10,  1.38, -0.75,  0.37, -1.43,  1.87, 
			#-0.58,  0.78, -2.11,  0.91,  0.50, -1.28, -0.16, -0.26, -0.10,  0.72, -2.81, -0.24,  1.39,  2.41,  0.27,  1.53, 
			#-0.61, -0.52,  0.57,  0.66, -0.62, -0.49, -0.32,  1.64, -0.30,  0.30,  0.30, -0.93, -0.15,  0.47, -0.00, -2.36, 
			#0.53,  2.08, -1.32,  1.18,  0.37, -0.63, -0.76, -2.81, -1.66, -0.22,  0.70, -1.17,  0.02,  0.46, -0.31,  0.64, 
			#-1.06, -0.62,  0.19,  2.00, -0.05,  0.08,  0.31, -1.14, -0.92, -2.06, -0.04, -0.05, -0.83, -0.09, -1.86,  1.45, 
			#1.87, -1.26, -0.78, -0.01,  0.31, -0.72,  1.04,  1.21, -0.19, -0.34, -2.37,  0.56, -0.64,  0.87,  1.85, -0.08, 
			#-1.81, -0.35,  1.07, -0.22, -1.41,  0.38, -1.29, -0.68, -2.29,  1.67, -0.39, -0.58,  0.62, -0.10, -0.11, -1.28, 
			#0.08, -1.36,  0.68, -0.54,  0.14, -2.17,  0.61,  1.09,  0.77, -0.99,  1.16, -0.76,  1.70, -0.42, -1.38, -0.74, 
			#-0.61,  0.69,  0.77, -0.05,  0.93, -0.65,  1.25,  0.14, -2.56, -0.14, -0.88, -0.77,  1.95, -1.03,  0.74,  0.55, 
			#1.83,  0.24, -0.17, -1.51,  1.71,  1.47, -0.26, -0.87,  1.09,  0.12, -0.21, -0.07,  0.78,  0.58, -0.70,  1.88, 
			#-0.47, -1.19,  0.63, -0.72,  0.09,  0.05,  1.07,  0.27, -1.22,  0.02, -1.24,  0.09,  2.66,  0.67,  0.41,  2.48, 
			#-2.11,  0.43, -0.86, -0.41,  0.86,  1.74,  0.75,  1.67, -0.02, -1.20, -0.02,  0.83, -0.14, -0.35, -0.67,  1.92, 
			#1.69,  2.13,  1.59,  1.06,  0.72,  0.09,  0.02,  1.92, -0.20, -0.85, -0.52, -0.77,  1.40,  1.02, -1.64, -0.08, 
			#1.47, -0.83,  0.98,  0.70,  0.46,  0.72,  0.28, -0.54,  0.23,  0.64,  0.73, -1.53, -0.38,  1.47, -1.17,  3.01, 
			#-1.79,  2.70, -1.06,  0.02, -0.26,  0.79, -1.49,  0.74,  0.19,  0.53, -0.70, -1.17,  1.40, -1.68, -0.14, -0.91, 
			#-0.45, -1.71, -0.38,  2.52, -0.95, -0.21,  1.20, -0.70,  0.26,  0.29,  1.30, -0.37,  0.15,  2.01, -1.03,  0.84, 
			#0.06,  0.40,  0.21,  1.74,  0.67, -1.77, -0.35,  0.16, -0.38,  1.30,  1.04, -0.21,  1.18, -1.92,  0.91,  2.58, 
			#-0.18,  1.55, -0.30,  1.00, -0.60,  0.84,  0.14,  0.11,  0.95, -0.87,  0.04, -0.89,  1.77, -1.63, -0.92,  0.05, 
			#-0.14, -0.73,  1.15,  1.33, -0.86, -0.53,  0.20,  1.85,  0.43,  1.46, -1.12,  0.20, -1.55, -1.12, -0.32,  0.46, 
			#1.12, -1.47,  0.29, -0.84,  1.00, -0.19, -0.99, -0.39,  1.02,  1.13, -0.15,  2.01, -0.01, -2.30,  1.61, -0.32, 
			#-1.14, -1.75, -1.60, -0.10,  0.44, -0.65, -0.33, -0.92, -0.45, -0.40,  0.30, -0.07, -0.79,  1.47, -0.91, -1.48, 
			#-0.58,  0.55,  0.31, -1.08,  2.73, -0.29, -2.20, -0.35, -1.87, -0.82,  0.58,  1.16, -2.10, -0.02, -0.18, -1.86, 
			#-0.83, -0.43,  0.60,  1.07,  0.27,  0.26, -1.78,  1.38,  0.97,  0.38,  0.57, -0.36,  0.77, -1.57, -0.50, -0.14, 
			#-1.49, -0.83,  0.08,  1.45, -0.36, -0.76, -0.13,  0.79,  0.05, -1.17, -1.40, -0.16, -0.61,  2.10, -1.35, -0.35, 
			#0.31,  0.32,  1.77,  1.18,  0.81, -0.49,  1.44, -0.98,  0.11, -0.18,  0.73,  0.56,  1.58, -0.68, -0.26, -0.82, 
			#-0.11, -0.59, -1.19, -0.49, -0.33,  0.97,  0.48,  0.97,  0.32, -2.13,  0.48,  1.00,  0.56,  1.60, -1.32, -0.15, 
			#1.20,  1.25,  0.91, -1.23, -0.12,  0.55,  0.13,  1.25, -0.38, -0.14,  0.05, -0.27,  1.12,  2.61,  1.29, -1.36, 
			#0.86,  0.28, -1.86, -1.93,  0.57,  0.16,  2.35,  0.88, -0.86,  1.76,  0.21,  0.40,  0.50, -0.21, -0.74,  0.34, 
			#-0.21,  0.07,  0.90,  0.41,  0.38,  0.40, -0.17, -0.56,  0.58, -1.44,  1.21,  1.26,  0.14,  0.80,  0.22,  1.04, 
			#-0.21, -0.01,  0.51,  0.70, -1.19,  0.50, -0.61, -0.35, -0.77, -2.00, -0.43,  0.49, -0.14,  0.56,  0.21,  1.11, 
			#0.21,  0.88,  0.18, -0.83, -1.34,  0.83, -0.69, -0.54,  0.03,  2.16,  0.18, -1.03, -0.69,  0.73, -1.80,  0.04, 
			#0.16,  0.88, -0.54,  1.86, -0.29, -0.96,  0.81, -0.92, -2.72, -0.76, -0.35,  0.06, -0.08,  0.26, -1.28, -0.09, 
			#2.16,  0.39,  1.23,  1.04,  0.99, -0.40,  0.21, -1.00, -0.58, -2.03, -0.44,  0.34, -0.98,  1.03, -0.82, -0.65, 
			#1.70, -0.97,  0.23, -0.34,  0.20,  0.46, -0.99,  0.32,  0.43,  1.49, -0.69, -0.91, -1.81, -0.15,  1.24, -0.76, 
			#0.35, -0.67, -0.25,  0.72, -0.73, -1.88, -0.56, -0.45, -0.82, -0.69,  0.45,  2.10, -0.20,  0.76, -0.13, -2.39, 
			#1.17, -0.43, -0.36,  1.38,  0.92,  0.17, -1.48,  1.26, -0.93,  0.91,  0.94, -0.40,  0.63, -1.44, -0.04, -0.34, 
			#-0.91, -0.70, -0.92, -0.35, -0.83,  1.11, -0.46,  1.03, -0.50, -0.36,  0.22, -0.27, -1.03, -0.38, -0.44, -0.97, 
			#0.74,  0.59,  0.26,  0.25, -0.58, -0.37, -0.17,  0.13, -0.42, -0.30,  0.92,  0.70, -0.76, -0.73, -0.06, -0.34, 
			#-1.27,  0.47, -1.05, -2.00,  1.16,  1.59,  0.07, -0.28,  1.53,  0.54, -1.03,  0.65, -0.81,  0.12, -0.93,  0.37, 
			#-0.90, -0.29, -0.38, -1.85, -1.14,  1.42,  0.23, -0.25,  1.23, -0.50, -1.23, -2.19, -0.56,  0.65,  1.20, -1.30, 
			#0.03,  0.87,  0.35, -1.17,  2.22, -0.25, -0.33,  0.19,  0.48,  0.87, -1.77, -2.22,  1.40,  0.85,  0.96,  0.19, 
			#0.85, -0.22, -1.76, -0.22,  0.50,  0.76, -1.04, -0.26,  1.42, -0.16,  0.01,  0.33,  1.08, -1.10,  1.54, -0.30, 
			#-1.76,  1.03, -1.41, -0.15, -0.13, -1.12,  2.03,  0.78, -1.26, -0.76,  0.38,  0.95, -0.95, -1.73,  0.11, -0.64, 
			#2.30,  1.37, -1.02,  1.68, -2.01, -0.09,  0.56, -0.22,  0.64, -0.78,  0.21,  0.21,  1.15, -1.58, -0.43,  1.20, 
			#2.42, -0.12, -1.57,  1.38,  0.59,  1.89,  1.38, -1.96, -0.93,  0.20, -1.46, -1.80,  0.32, -1.05, -1.36, -1.48, 
			#-1.87,  0.68, -0.44, -0.47,  0.28,  0.07, -0.83, -0.95,  0.32, -0.68,  0.18, -1.21, -0.93,  0.20,  0.45,  0.05, 
			#0.10, -1.13,  0.60, -0.93, -0.18,  1.69, -0.84,  0.64,  1.04, -0.19, -2.14, -2.01, -0.76,  0.37,  0.46,  0.73, 
			#0.55,  1.43,  0.53, -0.51,  1.81, -0.80, -0.53, -0.81,  1.43,  0.11, -1.30,  1.13, -0.40, -1.61,  0.38, -0.16, 
			#1.19,  1.32,  0.08,  1.06, -1.25, -0.21, -0.27, -0.50, -0.76,  0.31,  0.78,  0.40,  0.15,  0.98, -1.62, -0.21, 
			#0.57,  1.99,  1.78,  1.51,  0.55,  0.02, -0.51, -0.86,  2.08, -1.93, -0.51,  0.95, -0.12,  1.86, -0.50, -2.35, 
			#-0.83, -0.66,  0.99,  0.04, -0.08, -0.40,  1.09, -0.90, -0.51, -0.68, -0.67,  0.53, -0.88, -2.41,  2.17,  0.39, 
			#-0.54, -0.17,  0.26, -1.27,  0.46, -0.90,  0.63, -2.28, -2.06,  0.32, -0.74, -0.17, -0.56, -0.99, -0.81, -0.51, 
			#-1.27,  1.23, -1.19, -1.15,  0.13,  0.41, -1.13, -0.14, -1.16, -1.33, -1.34, -0.90,  0.40,  1.22, -0.15, -1.51, 
			#0.55, -1.02,  0.13, -0.92,  0.22,  0.02, -0.65,  0.51, -1.37, -0.69,  0.27, -0.04, -0.14,  0.92,  0.62,  0.19, 
			#-1.69, -0.62,  0.37,  0.53,  0.27, -0.44,  0.03,  0.80, -1.72, -0.06,  1.56,  0.02,  1.02,  0.35, -0.01,  2.56, 
			#0.96,  0.71,  0.39, -0.49,  0.10,  0.14, -0.11, -0.75,  1.55,  0.26,  0.68, -0.03, -0.63,  0.51, -0.60, -0.82, 
			#0.16, -1.72, -0.29,  0.88, -2.20,  1.07,  0.41,  0.80,  1.44,  0.62,  0.96, -0.81,  0.70, -0.60,  0.20,  1.33, 
			#1.91,  0.42,  1.04,  0.71, -1.73, -1.29, -0.05,  0.48,  0.58,  0.29, -0.56,  0.91,  1.11,  0.61, -1.22, -0.74, 
			#0.91, -1.22,  0.12, -0.97, -0.56, -0.21,  0.95,  0.65, -2.26,  1.12, -0.07,  0.37, -0.84,  0.81, -2.10,  1.05, 
			#-0.07, -0.65,  0.46, -0.98,  0.93, -0.04, -0.02, -0.29, -0.72, -1.44, -1.27, -1.50, -0.23, -0.83, -0.16, -0.18, 
			#-1.21,  0.21,  0.02,  0.12, -1.04, -2.03,  2.14, -0.43,  0.41, -0.21,  0.28, -0.20, -0.41,  0.03,  0.15, -0.72, 
			#-0.51, -0.24,  0.43, -0.48, -0.93,  0.92,  1.20, -1.89,  0.43,  0.62, -0.84, -0.98, -0.48, -0.32, -0.57, -0.50, 
			#-0.10, -2.19, -0.39, -0.94,  1.10, -2.19,  1.76, -2.06,  2.26,  1.37, -0.11,  1.81,  1.72,  0.83,  1.35, -2.13, 
			#0.04,  0.25,  0.36,  1.16, -1.66, -0.80,  0.05,  1.15, -1.28, -1.74, -1.16,  1.02,  1.27,  0.39,  0.84,  2.66, 
			#-0.70, -0.78, -1.61, -0.64, -1.02, -0.45,  1.24, -1.04,  1.00, -0.52, -0.48, -0.12,  2.87, -0.01, -0.20, -0.80, 
			#1.04, -0.23,  0.08, -1.44,  0.45, -0.55, -0.12, -1.41, -0.90, -0.05,  0.57,  0.67,  0.69, -0.49, -1.08, -0.11, 
			#0.57,  0.60, -0.80,  0.67, -1.27,  0.91,  0.09,  2.18,  0.19,  0.88,  1.04,  0.71,  0.92, -0.67, -0.66,  0.64, 
			#-0.24, -1.87,  0.38,  0.05, -0.17,  0.30, -0.20,  0.79,  0.44,  0.05,  0.86, -0.75, -0.18, -0.01,  0.03,  1.51, 
			#-0.29,  0.46,  0.65,  0.21,  0.66,  1.22, -0.69,  0.87, -1.68, -0.67, -0.30,  1.99,  0.24, -0.17, -0.28, -1.27, 
			#1.30, -0.58,  1.20, -1.71,  0.24,  1.01, -0.78,  0.68,  0.18,  0.21,  0.42,  0.43,  0.95,  0.89,  0.47,  0.41, 
			#1.66, -0.28,  0.87,  0.20,  1.52,  0.06, -0.12, -0.97, -1.61,  1.47,  1.68, -0.68,  0.82,  0.42, -0.74, -0.14, 
			#1.65, -0.83,  0.72,  1.39,  1.61, -0.52,  0.60, -0.53,  0.87,  0.79,  1.10, -1.48, -0.54, -1.36, -1.66, -0.29, 
			#-0.49,  0.14, -0.30,  1.23, -0.23,  1.20,  0.08, -0.30, -1.68,  1.33,  0.33, -0.70, -0.62, -0.65,  0.14, -0.45, 
			#1.16,  0.74, -1.18,  0.94, -2.24,  1.88, -1.75, -0.89, -0.50,  0.65,  0.11,  1.24,  1.18,  1.03,  0.89,  0.64, 
			#-0.39,  0.49,  0.58, -0.73, -0.57,  0.68, -1.15, -0.58,  0.98,  2.24, -1.08, -0.81,  1.40, -0.52, -0.95,  0.93, 
			#-0.86,  0.69,  0.23,  0.07, -1.04,  0.21, -1.43,  0.99,  0.45, -0.61, -0.05,  1.29, -0.04, -0.60, -1.33, -1.34, 
			#0.90,  0.55, -1.08, -0.95,  0.45, -0.57, -1.49, -1.22,  0.07,  0.47, -0.06, -0.50, -1.24, -0.34, -0.14, -1.20, 
			#0.72,  0.11, -1.45, -1.11,  1.02,  0.15, -0.78,  2.54, -1.09,  0.40, -0.19, -0.61, -1.79,  0.98, -2.37, -1.57, 
			#0.07, -0.50,  1.73,  0.34, -0.94,  1.01, -1.57,  0.41, -0.66,  0.33, -0.92, -0.42, -1.15,  0.03, -1.47,  0.09, 
			#1.97,  0.08,  1.18, -1.37, -0.14, -0.67, -1.58,  2.03,  0.02,  1.95,  1.43, -0.99,  0.81,  0.88,  0.06,  1.26, 
			#-2.52,  2.70,  1.25,  2.20, -0.46,  0.46,  1.82, -1.16, -0.27, -0.90,  1.65,  0.75,  0.71,  0.39, -1.12,  0.76, 
			#-0.60, -1.68,  0.49, -0.52,  0.06, -0.17,  0.63, -2.95,  0.81, -1.63, -0.68, -0.58,  0.81,  0.03,  0.73,  1.36, 
			#-0.14,  1.20,  0.42, -0.64, -0.39,  1.41,  1.35, -0.87,  1.17,  0.16,  0.52,  1.21,  0.91,  2.53,  1.98, -0.10, 
			#-1.14,  0.77, -0.51, -1.66, -0.54,  0.59,  0.89,  0.82, -0.26, -0.07,  1.30,  0.81, -1.69,  0.84, -1.95,  0.44, 
			#0.36,  0.87, -0.68, -1.37, -0.26, -0.10,  0.74,  0.37, -0.14,  0.01, -0.20,  1.73,  1.17,  0.42, -0.82, -0.59, 
			#-0.53,  0.61,  0.47,  1.03,  0.62,  0.60, -1.00,  0.59,  0.05,  0.33, -0.49, -0.80, -0.43,  0.84, -0.60, -1.37, 
			#0.32,  0.81,  1.80,  0.28, -1.27, -1.09, -0.01, -0.22, -0.72, -0.72, -0.05,  1.95,  0.25, -0.22,  0.36, -1.28, 
			#-1.31, -1.08, -0.31, -1.01, -2.15,  0.14,  1.27, -0.77,  0.96,  0.11, -0.57, -0.70, -0.66,  0.90, -0.41,  0.73, 
			#0.31, -1.33,  0.72, -0.20,  0.63, -0.57, -1.18, -0.95,  1.34, -1.22, -2.01,  0.18,  0.78,  0.03, -0.04,  0.20, 
			#0.74,  0.07,  0.29, -1.22, -0.86, -0.43,  1.01, -1.12,  0.19, -1.68,  0.31, -0.25,  0.17, -0.48,  0.98, -0.87, 
			#0.57,  1.17, -0.27,  2.20, -1.14, -0.51, -0.44, -1.87,  1.63,  1.06,  1.66, -1.62, -1.66,  1.11,  0.95,  0.04, 
			#0.36, -0.04,  0.51, -0.07, -0.23, -1.04, -0.23, -0.36, -0.85,  0.24, -0.98,  1.19,  0.90, -1.73,  0.01,  1.29, 
			#-0.42,  0.34,  1.18, -0.61, -0.27, -0.17,  1.00, -0.27, -2.17, -0.85, -1.03,  0.49,  1.50,  1.82,  0.05, -1.77, 
			#-0.12, -1.69, -0.98,  1.52,  0.14,  0.78,  0.12,  1.17, -0.43,  1.13,  1.75, -0.57, -1.74,  1.48, -0.78,  0.36, 
			#-0.20, -0.30,  0.86,  0.32,  0.36,  1.44,  1.42,  0.35, -0.77,  0.25, -1.17,  0.36,  0.14, -0.77, -1.51, -0.58, 
			#0.77, -0.56,  0.56, -2.65, -1.34,  0.67,  2.52, -0.37, -0.38,  1.32,  1.30, -0.54, -0.52, -0.19,  1.71,  1.60, 
			#-0.60,  0.79, -1.06,  0.54,  0.30, -0.35,  0.77, -0.49,  1.98,  0.63,  0.38, -1.45, -0.06, -0.05,  1.64,  0.05, 
			#-0.90, -0.28,  0.12,  2.01, -0.28,  1.95,  1.06,  1.07, -0.96,  0.47, -0.56,  0.84, -0.88, -0.37,  0.27,  0.67, 
			#-0.56, -0.42, -0.89, -1.05, -0.32, -0.15, -0.77,  1.06,  0.19,  1.01,  0.07,  0.74,  0.85,  2.01, -1.17,  0.69, 
			#1.39, -1.03, -0.21, -1.21, -1.28, -1.42,  0.35,  0.46,  0.52, -0.62, -1.36, -0.53,  1.50, -1.43,  0.62,  0.15, 
			#-1.00, -0.05, -0.19,  3.20, -0.78, -0.58, -0.87,  0.37, -0.54, -0.56, -0.84,  1.40, -1.42,  0.03,  0.77,  1.20, 
			#-0.09, -2.14,  0.25,  0.71, -0.08, -1.15, -0.01, -0.88, -0.28, -1.13,  0.44,  1.45,  0.94,  0.77,  0.32,  0.28, 
			#1.73, -1.73,  0.50, -1.30,  0.73,  0.73, -0.16,  0.54, -1.37,  0.68,  0.89, -1.53,  2.57, -0.27,  0.00,  0.01, 
			#-0.61, -2.39, -0.04, -0.00,  0.98,  1.46,  0.00,  0.56, -1.03, -0.95, -0.29,  0.61, -2.82,  0.90,  0.78, -0.26, 
			#2.37,  1.38, -0.67, -1.69, -0.80, -0.26,  0.10, -0.56,  1.07,  0.73, -0.22,  1.67,  0.63,  0.89,  0.10,  0.30, 
			#0.12,  0.04,  1.37, -1.40, -1.44, -0.28, -0.33, -0.37,  0.20,  0.11, -0.07, -1.21, -1.42, -0.05,  0.12, -0.55, 
			#-0.08,  0.29,  0.03, -1.74,  0.06, -1.03, -1.27,  1.91, -0.31, -0.52, -0.58, -1.32,  2.42,  1.06,  0.72,  0.27, 
		#)
		#src = gr.vector_source_f(src_data)
		#sqr = licorne.hachoir_f(freq=2500000, samplerate=8000000, fft_size=1024, window_type=1)
		#self.tb.connect(src, sqr)
		#self.tb.run ()
		## check data
		
	#def test_001_t (self):
		#samp_rate = 32000
		#self.noise = noise = .005
		#self.ampl = ampl = .4
		#self.gr_sig_source_x0 = gr.sig_source_f(samp_rate, gr.GR_COS_WAVE, 350, ampl, 0)
		#self.gr_sig_source_x = gr.sig_source_f(samp_rate, gr.GR_COS_WAVE, 1000, ampl, 0)
		#self.gr_noise_source_x = gr.noise_source_f(gr.GR_GAUSSIAN, noise, 0)
		#self.gr_add_xx = gr.add_vff(1)
		#sqr = licorne.hachoir_f(freq=2500000, samplerate=samp_rate, fft_size=1024, window_type=1)
		#self.tb.connect((self.gr_sig_source_x0, 0), (self.gr_add_xx, 0))
		#self.tb.connect((self.gr_sig_source_x, 0), (self.gr_add_xx, 1))
		#self.tb.connect((self.gr_noise_source_x, 0), (self.gr_add_xx, 2))
		#self.tb.connect((self.gr_add_xx, 0), sqr)
		#self.tb.run ()
		## check data


if __name__ == '__main__':
	gr_unittest.run(qa_hachoir_f, "qa_hachoir_f.xml")
