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
// Created by Vetle Wegner Ingeberg on 05/03/2026.
//

#include "histogram/ThreadSafeHistograms.h"

ThreadSafeHistogram1D ThreadSafeHistograms::Get1D(const std::string &name)
{
    auto p = Get(map1d, name);
    return {this, p->mutex, p->object, min_buffer, max_buffer};
}

ThreadSafeHistogram2D ThreadSafeHistograms::Get2D(const std::string &name)
{
    auto p = Get(map2d, name);
    return {this, p->mutex, p->object, min_buffer, max_buffer};
}

ThreadSafeHistogram3D ThreadSafeHistograms::Get3D(const std::string &name)
{
    auto p = Get(map3d, name);
    return {this, p->mutex, p->object, min_buffer, max_buffer};
}

ThreadSafeHistogram1D ThreadSafeHistograms::Create1D( const std::string& name,  /*!< The name of the new histogram. */
                                    const std::string& title, /*!< The title of teh new histogram. */
                                    Axis::index_t channels,   /*!< The number of regular bins. */
                                    Axis::bin_t left,         /*!< The lower edge of the lowest bin.  */
                                    Axis::bin_t right,        /*!< The upper edge of the highest bin. */
                                    const std::string& xtitle /*!< The title of the x axis. */)
{
    std::lock_guard lock(mutex);
    try {
        return Get1D(name);
    } catch ( std::out_of_range &e ){
        // The histogram doesn't exist, we will create it now.
        p1d hist = new ThreadSafeHistogramDetails::protected_object(histograms.Create1D(name, title, channels, left, right, xtitle));
        map1d[name] = hist;
        return Get1D(name);
    }
}

ThreadSafeHistogram2D ThreadSafeHistograms::Create2D( const std::string& name,   /*!< The name of the new histogram. */
                                                        const std::string& title,  /*!< The title of teh new histogram. */
                                                        Axis::index_t xchannels,   /*!< The number of regular bins on the x axis. */
                                                        Axis::bin_t xleft,         /*!< The lower edge of the lowest bin on the x axis. */
                                                        Axis::bin_t xright,        /*!< The upper edge of the highest bin on the x axis. */
                                                        const std::string& xtitle, /*!< The title of the x axis. */
                                                        Axis::index_t ychannels,   /*!< The number of regular bins on the y axis. */
                                                        Axis::bin_t yleft,         /*!< The lower edge of the lowest bin on the y axis. */
                                                        Axis::bin_t yright,        /*!< The upper edge of the highest bin on the y axis. */
                                                        const std::string& ytitle  /*!< The title of the y axis. */)
{
    std::lock_guard lock(mutex);
    try {
        return Get2D(name);
    } catch ( std::out_of_range &e ){
        // The histogram doesn't exist, we will create it now.
        p2d hist =
                new ThreadSafeHistogramDetails::protected_object(
                        histograms.Create2D(name, title,
                                                   xchannels, xleft, xright, xtitle,
                                                   ychannels, yleft, yright, ytitle));
        map2d[name] = hist;
        return Get2D(name);
    }
}

ThreadSafeHistogram3D ThreadSafeHistograms::Create3D( const std::string& name,   /*!< The name of the new histogram. */
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
                                const std::string& ztitle  /*!< The title of the z axis. */)
{
    std::lock_guard lock(mutex);
    try {
        return Get3D(name);
    } catch (std::out_of_range &e) {
        // The histogram doesn't exist, we will create it now.
        p3d hist = new ThreadSafeHistogramDetails::protected_object(
                histograms.Create3D(name, title,
                                    xchannels, xleft, xright, xtitle,
                                    ychannels, yleft, yright, ytitle,
                                    zchannels, zleft, zright, ztitle));
        map3d[name] = hist;
        return Get3D(name);
    }
}


void ThreadSafeHistograms::force_flush() {
    std::lock_guard lock(mutex);
    for ( auto hist : list1d ) {
        hist->force_flush();
    }
    for ( auto mat : list2d ) {
        mat->force_flush();
    }
    for ( auto cube : list3d ) {
        cube->force_flush();
    }
}
