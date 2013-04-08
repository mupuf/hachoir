/* -*- c++ -*- */

#define GTSRC_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "gtsrc_swig_doc.i"

%{
#include "gtsrc/hachoir_c.h"
%}

%include "gtsrc/hachoir_c.h"
GR_SWIG_BLOCK_MAGIC2(gtsrc, hachoir_c);
