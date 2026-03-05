// Copyright (c) 2026. Vetle Wegner Ingeberg/University of Oslo.
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
//
// Created by Vetle Wegner Ingeberg on 05/03/2026.
//

#include "histogram/ThreadSafeHistogram1D.h"

ThreadSafeHistogram1D::ThreadSafeHistogram1D(ThreadSafeHistograms* _histograms, std::mutex &_mutex, Histogram1D *_histogram,
                          const size_t &_min_buffer, const size_t &_max_buffer)
        : ThreadSafeHistogram( _histograms, _mutex, _histogram, _min_buffer, _max_buffer )
{
    histograms->List1D(this);
}

ThreadSafeHistogram1D::~ThreadSafeHistogram1D() {
    histograms->Remove1D(this);
}
