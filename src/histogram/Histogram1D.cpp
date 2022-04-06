// Copyright (c) 2022. Vetle Wegner Ingeberg/University of Oslo.
// All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/*
 * Histogram1D.cpp
 *
 *  Created on: 10.03.2010
 *      Author: Alexander Bürger
 */

#include "Histogram1D.h"

#include <iostream>


#ifdef H1D_USE_BUFFER
const unsigned int Histogram1D::buffer_max;
#endif /* H1D_USE_BUFFER */

// ########################################################################

Histogram1D::Histogram1D( const std::string& name, const std::string& title,
                          Axis::index_t c, Axis::bin_t l, Axis::bin_t r, const std::string& xt,
                          const std::string& path)
    : Named( name, title, path )
    , xaxis( name+"_xaxis", c, l, r, xt )
    , data( 0 )
{
#ifdef H1D_USE_BUFFER
  buffer.reserve(buffer_max);
#endif /* H1D_USE_BUFFER */

  data = new data_t[xaxis.GetBinCountAll()];
  Reset();
}

// ########################################################################

Histogram1D::~Histogram1D()
{
  delete data;
}

// ########################################################################

void Histogram1D::Add(const Histogram1Dp other, data_t scale)
{
  if( !other
//      || other->GetName() != GetName() // This shouldn't be a requirement.
      || other->GetAxisX().GetLeft() != xaxis.GetLeft()
      || other->GetAxisX().GetRight() != xaxis.GetRight()
      || other->GetAxisX().GetBinCount() != xaxis.GetBinCount() )
    return;

#ifdef H1D_USE_BUFFER
  other->FlushBuffer();
    FlushBuffer();
#endif /* H2D_USE_BUFFER */

  for(Axis::index_t i=0; i < xaxis.GetBinCountAll(); ++i)
    data[i] += scale * other->data[i];

  // Update total count
  entries += scale * other->entries;
}

// ########################################################################

Histogram1D::data_t Histogram1D::GetBinContent(Axis::index_t bin)
{
#ifdef H1D_USE_BUFFER
  FlushBuffer();
#endif /* H1D_USE_BUFFER */
  if( bin<xaxis.GetBinCountAll() ) {
    return data[bin];
  } else {
    return 0;
  }
}

// ########################################################################

void Histogram1D::FillDirect(Axis::bin_t x, data_t weight)
{
  entries += 1;
  data[xaxis.FindBin( x )] += weight;
}

// ########################################################################

#ifdef H1D_USE_BUFFER
void Histogram1D::FlushBuffer()
{
    if( !buffer.empty() ) {
        for(buffer_t::const_iterator it=buffer.begin(); it<buffer.end(); ++it)
            FillDirect(it->x, it->w);
        buffer.clear();
    }
}

#endif /* H1D_USE_BUFFER */

// ########################################################################

void Histogram1D::Reset()
{
#ifdef H1D_USE_BUFFER
  buffer.clear();
#endif /* H1D_USE_BUFFER */
  for(Axis::index_t i=0; i < xaxis.GetBinCountAll(); ++i)
    data[i] = 0;
  entries = 0;
}
