// Copyright (c) 2022, 2026. Vetle Wegner Ingeberg/University of Oslo.
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
// Created by Vetle Wegner Ingeberg on 11/03/2022.
//

#ifndef THREADSAFEHISTOGRAMS_H
#define THREADSAFEHISTOGRAMS_H


#include <histogram/Histograms.h>
#include <histogram/Histogram1D.h>
#include <histogram/Histogram2D.h>
#include <histogram/Histogram3D.h>

#include <histogram/ThreadSafeHistogram1D.h>
#include <histogram/ThreadSafeHistogram2D.h>
#include <histogram/ThreadSafeHistogram3D.h>

/*!
 * Thread safe histograms are histograms where the underlying memory for the histogram are stored thread safely.
 * Each thread will get an "adapter" class that buffers the entries in a vector. If the buffer is larger than
 * the min flush size the adapter class will try to lock a mutex, if failed it will continue filling the buffer
 * until the size is larger than the max flush size. Once this has been reached the adapter will wait until the
 * mutex is released and then flush its buffer.
 */

#include <string>
#include <map>
#include <list>
#include <vector>
#include <mutex>
#include <exception>
#include <stdexcept>


class ThreadSafeHistogram1D;
class ThreadSafeHistogram2D;
class ThreadSafeHistogram3D;

namespace ThreadSafeHistogramDetails {
    template<typename H>
    struct protected_object
    {
        std::mutex mutex;
        H object;
        protected_object(H _object) : mutex(), object(_object) {}
    };
}

class ThreadSafeHistograms
{
    friend class ThreadSafeHistogram1D;
    friend class ThreadSafeHistogram2D;
    friend class ThreadSafeHistogram3D;

private:

    Histograms histograms;
    std::mutex mutex;

    const size_t min_buffer;
    const size_t max_buffer;

    typedef ThreadSafeHistogramDetails::protected_object<Histogram1Dp>* p1d;
    typedef ThreadSafeHistogramDetails::protected_object<Histogram2Dp>* p2d;
    typedef ThreadSafeHistogramDetails::protected_object<Histogram3Dp>* p3d;

    std::map<std::string, p1d> map1d;
    std::map<std::string, p2d> map2d;
    std::map<std::string, p3d> map3d;

    std::list<ThreadSafeHistogram1D *> list1d;
    std::list<ThreadSafeHistogram2D *> list2d;
    std::list<ThreadSafeHistogram3D *> list3d;


    template<typename T>
    static typename T::value_type::second_type Get(T map, const std::string &name)
    {
        const auto obj = map.find(name);
        if ( obj != map.end() ){
            return obj->second;
        } else {
            throw std::out_of_range("Not defined");
        }
    }

    ThreadSafeHistogram1D Get1D(const std::string &name);

    ThreadSafeHistogram2D Get2D(const std::string &name);

    ThreadSafeHistogram3D Get3D(const std::string &name);

protected: // Note these functions are only ran from the ThreadSafeHistogram1D/2D/3D
    void List1D(ThreadSafeHistogram1D* hist) {
        // Note these functions are only ran from ThreadSafeHistogram1D constructor.
        // The constructor will only run if the mutex is already locked. This function should not lock the mutex.
        list1d.push_back(hist);
    }

    void Remove1D(ThreadSafeHistogram1D* hist) {
        std::lock_guard<std::mutex> lock(mutex);
        list1d.remove_if([hist](auto &el){ return el == hist; });
    }

    void List2D(ThreadSafeHistogram2D* mat) {
        // Note these functions are only ran from ThreadSafeHistogram2D constructor.
        // The constructor will only run if the mutex is already locked. This function should not lock the mutex.
        list2d.push_back(mat);
    }

    void Remove2D(ThreadSafeHistogram2D* mat) {
        std::lock_guard<std::mutex> lock(mutex);
        list2d.remove_if([mat](auto &el){ return el == mat; });
    }

    void List3D(ThreadSafeHistogram3D* cube) {
        // Note these functions are only ran from ThreadSafeHistogram3D constructor.
        // The constructor will only run if the mutex is already locked. This function should not lock the mutex.
        list3d.push_back(cube);
    }

    void Remove3D(ThreadSafeHistogram3D* cube) {
        std::lock_guard<std::mutex> lock(mutex);
        list3d.remove_if([cube](auto &el){ return el == cube; });
    }

public:

    ThreadSafeHistograms(const size_t &min_buf = 1024, const size_t &max_buf = 16384)
        : min_buffer( min_buf ), max_buffer( max_buf ){}

    ~ThreadSafeHistograms()
    {
        for ( auto &hist : map1d ){
            delete hist.second;
        }
        for ( auto &hist : map2d ){
            delete hist.second;
        }
        for ( auto &hist : map3d ){
            delete hist.second;
        }
    }

    ThreadSafeHistogram1D Create1D( const std::string& name,  /*!< The name of the new histogram. */
                                    const std::string& title, /*!< The title of teh new histogram. */
                                    Axis::index_t channels,   /*!< The number of regular bins. */
                                    Axis::bin_t left,         /*!< The lower edge of the lowest bin.  */
                                    Axis::bin_t right,        /*!< The upper edge of the highest bin. */
                                    const std::string& xtitle /*!< The title of the x axis. */);


    ThreadSafeHistogram2D Create2D( const std::string& name,   /*!< The name of the new histogram. */
                                    const std::string& title,  /*!< The title of teh new histogram. */
                                    Axis::index_t xchannels,   /*!< The number of regular bins on the x axis. */
                                    Axis::bin_t xleft,         /*!< The lower edge of the lowest bin on the x axis. */
                                    Axis::bin_t xright,        /*!< The upper edge of the highest bin on the x axis. */
                                    const std::string& xtitle, /*!< The title of the x axis. */
                                    Axis::index_t ychannels,   /*!< The number of regular bins on the y axis. */
                                    Axis::bin_t yleft,         /*!< The lower edge of the lowest bin on the y axis. */
                                    Axis::bin_t yright,        /*!< The upper edge of the highest bin on the y axis. */
                                    const std::string& ytitle  /*!< The title of the y axis. */);

    ThreadSafeHistogram3D Create3D( const std::string& name,   /*!< The name of the new histogram. */
                                    const std::string& title,  /*!< The title of teh new histogram. */
                                    Axis::index_t xchannels,   /*!< The number of regular bins on the x axis. */
                                    Axis::bin_t xleft,         /*!< The lower edge of the lowest bin on the x axis. */
                                    Axis::bin_t xright,        /*!< The upper edge of the highest bin on the x axis. */
                                    const std::string& xtitle, /*!< The title of the x axis. */
                                    Axis::index_t ychannels,   /*!< The number of regular bins on the y axis. */
                                    Axis::bin_t yleft,         /*!< The lower edge of the lowest bin on the y axis. */
                                    Axis::bin_t yright,        /*!< The upper edge of the highest bin on the y axis. */
                                    const std::string& ytitle, /*!< The title of the y axis. */
                                    Axis::index_t zchannels,   /*!< The number of regular bins on the z axis. */
                                    Axis::bin_t zleft,         /*!< The lower edge of the lowest bin on the z axis. */
                                    Axis::bin_t zright,        /*!< The upper edge of the highest bin on the z axis. */
                                    const std::string& ztitle  /*!< The title of the z axis. */);

    void force_flush();

    Histograms &GetHistograms(){ return histograms; }

};


#endif // THREADSAFEHISTOGRAMS_H
