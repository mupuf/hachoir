/* -*- c++ -*- */

#define LICORNE_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "licorne_swig_doc.i"

%{
#include "licorne/hachoir_f.h"
%}

%include "licorne/hachoir_f.h"
GR_SWIG_BLOCK_MAGIC2(licorne, hachoir_f);
