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
 *  Created on: 10.03.2010
 *      Author: Alexander Bürger
 */

#include "Histogram2D.h"

#include <iostream>

#ifdef H2D_USE_BUFFER
const unsigned int Histogram2D::buffer_max;
#endif /* H2D_USE_BUFFER */

// ########################################################################

Histogram2D::Histogram2D( const std::string& name, const std::string& title,
                          Axis::index_t ch1, Axis::bin_t l1, Axis::bin_t r1, const std::string& xt,
                          Axis::index_t ch2, Axis::bin_t l2, Axis::bin_t r2, const std::string& yt,
                          const std::string& path)
    : Named( name, title, path )
    , xaxis( name+"_xaxis", ch1, l1, r1, xt )
    , yaxis( name+"_yaxis", ch2, l2, r2, yt )
    , entries( 0 )
#ifndef USE_ROWS
    , data( nullptr )
#else
, rows( nullptr )
#endif
{
#ifdef H2D_USE_BUFFER
  buffer.reserve(buffer_max);
#endif /* H2D_USE_BUFFER */

#ifndef USE_ROWS
  data = new data_t[xaxis.GetBinCountAll()*yaxis.GetBinCountAll()];
#else
  rows = new data_t*[yaxis.GetBinCountAll()];
    for(Axis::index_t y=0; y<yaxis.GetBinCountAll(); ++y)
        rows[y] = new data_t[xaxis.GetBinCountAll()];
#endif
  Reset();
}

// ########################################################################

Histogram2D::~Histogram2D()
{
#ifndef USE_ROWS
  delete data;
#else
  for(Axis::index_t y=0; y < yaxis.GetBinCountAll(); ++y)
        delete rows[y];
    delete rows;
#endif
}

// ########################################################################

void Histogram2D::Add(const Histogram2Dp &other, data_t scale)
{
  if( !other
      //|| other->GetName() != GetName()
      || other->GetAxisX().GetLeft() != xaxis.GetLeft()
      || other->GetAxisX().GetRight() != xaxis.GetRight()
      || other->GetAxisX().GetBinCount() != xaxis.GetBinCount()
      || other->GetAxisY().GetLeft() != yaxis.GetLeft()
      || other->GetAxisY().GetRight() != yaxis.GetRight()
      || other->GetAxisY().GetBinCount() != yaxis.GetBinCount() )
    throw std::runtime_error("Histograms '"+GetName()+"' and '"+other->GetName()+"' does not have the same dimentions.");

#ifdef H2D_USE_BUFFER
  other->FlushBuffer();
    FlushBuffer();
#endif /* H2D_USE_BUFFER */

#ifndef USE_ROWS
  for(Axis::index_t i=0; i<xaxis.GetBinCountAll()*yaxis.GetBinCountAll(); ++i)
    data[i] += scale * other->data[i];
#else
  for(Axis::index_t y=0; y<yaxis.GetBinCountAll(); ++y )
        for(Axis::index_t x=0; x<xaxis.GetBinCountAll(); ++x )
            rows[y][x] += scale*other->rows[y][x];
#endif
  // Update total count
  entries += scale * other->entries;
}

// ########################################################################

Histogram2D::data_t Histogram2D::GetBinContent(Axis::index_t xbin, Axis::index_t ybin)
{
#ifdef H2D_USE_BUFFER
  if( !buffer.empty() )
        FlushBuffer();
#endif /* H2D_USE_BUFFER */

  if( xbin<xaxis.GetBinCountAll() && ybin<yaxis.GetBinCountAll() ) {
#ifndef USE_ROWS
    return data[xaxis.GetBinCountAll()*ybin + xbin];
#else
    return rows[ybin][xbin];
#endif
  } else
    return 0;
}

// ########################################################################

void Histogram2D::SetBinContent(Axis::index_t xbin, Axis::index_t ybin, data_t c)
{
#ifdef H2D_USE_BUFFER
  if( !buffer.empty() )
        FlushBuffer();
#endif /* H2D_USE_BUFFER */

  if( xbin<xaxis.GetBinCountAll() && ybin<yaxis.GetBinCountAll() ) {
#ifndef USE_ROWS
    data[xaxis.GetBinCountAll()*ybin + xbin] = c;
#else
    rows[ybin][xbin] = c;
#endif
  }
}

// ########################################################################

void Histogram2D::FillDirect(Axis::bin_t x, Axis::bin_t y, data_t weight)
{
  const Axis::index_t xbin = xaxis.FindBin( x );
  const Axis::index_t ybin = yaxis.FindBin( y );
#ifndef USE_ROWS
  data[xaxis.GetBinCountAll()*ybin + xbin] += weight;
#else
  rows[ybin][xbin] += weight;
#endif
  entries += 1;
}

// ########################################################################

#ifdef H2D_USE_BUFFER
void Histogram2D::FlushBuffer()
{
    if( !buffer.empty() ) {
      for ( auto &v : buffer )
        FillDirect(v.x, v.y, v.w);
      buffer.clear();
    }
}
#endif /* H2D_USE_BUFFER */

// ########################################################################

void Histogram2D::Reset()
{
#ifdef H2D_USE_BUFFER
  buffer.clear();
#endif /* H2D_USE_BUFFER */
  for(Axis::index_t y=0; y<yaxis.GetBinCountAll(); ++y )
    for(Axis::index_t x=0; x<xaxis.GetBinCountAll(); ++x )
      SetBinContent( x, y, 0 );
  entries = 0;
}

// ########################################################################
// ########################################################################

#ifdef TEST_HISTOGRAM2D

//#include "RootWriter.h"
//#include <TFile>

int main(int argc, char* argv[])
{
    Histogram2D h("ho", "hohoho", 10,0,10,"xho", 10,0,40, "yho");
    h.Fill( 3,20, 7);
    h.Fill( 4,19, 6);
    h.Fill( 5,-2,1 );
    h.Fill( -1,-1, 10 );

    for(int iy=11; iy>=0; --iy) {
        for(int ix=0; ix<12; ++ix)
            std::cout << h.GetBinContent(ix, iy) << ' ';
        std::cout << std::endl;
    }
}

#endif