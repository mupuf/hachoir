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


#ifndef INCLUDED_LICORNE_HACHOIR_F_H
#define INCLUDED_LICORNE_HACHOIR_F_H

#include <licorne/api.h>
#include <gr_block.h>

namespace gr {
  namespace licorne {

    /*!
     * \brief <+description of block+>
     * \ingroup licorne
     *
     */
    class LICORNE_API hachoir_c : virtual public gr_block
    {
    public:
       typedef boost::shared_ptr<hachoir_c> sptr;

       /*!
        * \brief Return a shared_ptr to a new instance of licorne::hachoir_f.
        *
        * To avoid accidental use of raw pointers, licorne::hachoir_f's
        * constructor is in a private implementation
        * class. licorne::hachoir_f::make is the public interface for
        * creating new instances.
        */
       static sptr make(double freq, int samplerate, int fft_size, int window_type);
    };

  } // namespace licorne
} // namespace gr

#endif /* INCLUDED_LICORNE_HACHOIR_F_H */

