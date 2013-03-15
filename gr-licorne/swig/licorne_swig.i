/* -*- c++ -*- */

#define LICORNE_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "licorne_swig_doc.i"

%{
#include "licorne/hachoir_c.h"
%}

%include "licorne/hachoir_c.h"
GR_SWIG_BLOCK_MAGIC2(licorne, hachoir_c);
