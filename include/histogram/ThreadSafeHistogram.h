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

#ifndef THREADSAFEHISTOGRAM_H
#define THREADSAFEHISTOGRAM_H

#include <mutex>
#include <cstdint>

class ThreadSafeHistograms;

template<typename T>
class ThreadSafeHistogram
{
    friend class ThreadSafeHistograms;
protected:
    ThreadSafeHistograms* histograms;
private:
    std::mutex &mutex;
    T *histogram;

    const size_t min_buffer;
    const size_t max_buffer;
protected:
    typename T::buffer_t buffer;

private:
    void flush()
    {
        for ( auto &element : buffer ){
            histogram->FillDirect(element);
        }
        buffer.clear();
    }

    void try_flush()
    {
        if ( mutex.try_lock() ){
            flush();
            mutex.unlock();
        }
    }

protected:
    void check_buffer()
    {
        if ( buffer.size() < min_buffer )
            return;
        else if ( buffer.size() < max_buffer )
            try_flush();
        else
            force_flush();
    }

    void unsafe_flush() { flush(); }

    ThreadSafeHistogram(ThreadSafeHistograms* _histograms, std::mutex &_mutex, T *_histogram,
                        const size_t &_min_buffer = 1024, const size_t &_max_buffer = 16384)
        : histograms( _histograms )
        , mutex( _mutex )
        , histogram( _histogram )
        , min_buffer( _min_buffer )
        , max_buffer( _max_buffer )
    {
        buffer.reserve( max_buffer );
    }

public:
    ThreadSafeHistogram(ThreadSafeHistogram&) = delete;
    ThreadSafeHistogram(ThreadSafeHistogram&&)  = delete;

    ~ThreadSafeHistogram()
    {
        force_flush();
    }

    void force_flush()
    {
        std::lock_guard lock(mutex);
        flush();
    }

};

#endif //THREADSAFEHISTOGRAM_H