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

#ifndef THREADSAFEHISTOGRAM2D_H
#define THREADSAFEHISTOGRAM2D_H

#include <histogram/ThreadSafeHistogram.h>
#include <histogram/ThreadSafeHistograms.h>

class ThreadSafeHistogram2D : public ThreadSafeHistogram<Histogram2D>
{
private:
    friend class ThreadSafeHistograms;
protected:
    ThreadSafeHistogram2D(ThreadSafeHistograms* _histograms, std::mutex &_mutex, Histogram2D *_histogram,
                          const size_t &_min_buffer = 1024, const size_t &_max_buffer = 16384);
public:
    ~ThreadSafeHistogram2D();
    void Fill(const Axis::bin_t &x, const Axis::bin_t &y, const Histogram2D::data_t &n = 1)
    {
        buffer.emplace_back(x, y, n);
        check_buffer();
    }

};

#endif //THREADSAFEHISTOGRAM2D_H