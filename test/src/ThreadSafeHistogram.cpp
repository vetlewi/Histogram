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

//
// Created by Vetle Wegner Ingeberg on 20/10/2022.
//

#include <doctest/doctest.h>
#include <histogram/version.h>
#include <histogram/ThreadSafeHistograms.h>
#include <histogram/MamaWriter.h>

#include <thread>

#include <iostream>
#include <sstream>


TEST_SUITE_BEGIN( "ThreadSafeHistograms" );

static ThreadSafeHistograms histograms;

TEST_CASE( "Thread safe 1D histogram" ){

    ThreadSafeHistogram1D ts_hist = histograms.Create1D("hist", "hist title", 1024, 0, 1024, "x");
    Histogram1Dp hist = histograms.GetHistograms().Find1D("hist");

    CHECK( hist != nullptr );

    SUBCASE("Metadata") {
        CHECK(hist->GetName() == "hist");
        CHECK(hist->GetTitle() == "hist title");
        CHECK(hist->GetAxisX().GetName() == "hist_xaxis");
        CHECK(hist->GetAxisX().GetTitle() == "x");

        CHECK(hist->GetAxisX().GetLeft() == 0);
        CHECK(hist->GetAxisX().GetRight() == 1024);
        CHECK(hist->GetAxisX().GetBinWidth() == 1.0);

    }

    SUBCASE("Number of bins"){
        CHECK(hist->GetAxisX().GetBinCount() == 1024);
        CHECK(hist->GetAxisX().GetBinCountAll() == hist->GetAxisX().GetBinCount() + 2);
    }

    SUBCASE("Fill and lookup"){
        ts_hist.Fill(83);
        CHECK(hist->GetEntries() == 0);

        hist->Fill(83.5);
        ts_hist.force_flush();
        CHECK(hist->GetEntries() == 2);
        CHECK(hist->GetBinContent(hist->GetAxisX().FindBin(83.5)) == 2);
    }
}

TEST_CASE( "Thread safe 2D histogram" )
{

    ThreadSafeHistogram2D ts_mat = histograms.Create2D("mat", "mat title", 1024, 0, 1024, "x", 2048, 0, 2048, "y");
    Histogram2Dp mat = histograms.GetHistograms().Find2D("mat");

    CHECK( mat != nullptr );

    SUBCASE("Metadata") {
        CHECK(mat->GetName() == "mat");
        CHECK(mat->GetTitle() == "mat title");
        CHECK(mat->GetAxisX().GetName() == "mat_xaxis");
        CHECK(mat->GetAxisY().GetName() == "mat_yaxis");
        CHECK(mat->GetAxisX().GetTitle() == "x");
        CHECK(mat->GetAxisY().GetTitle() == "y");

        CHECK(mat->GetAxisX().GetLeft() == 0);
        CHECK(mat->GetAxisX().GetRight() == 1024);
        CHECK(mat->GetAxisX().GetBinWidth() == 1.0);

        CHECK(mat->GetAxisY().GetLeft() == 0);
        CHECK(mat->GetAxisY().GetRight() == 2048);
        CHECK(mat->GetAxisY().GetBinWidth() == 1.0);
    }

    SUBCASE("Number of bins") {
        CHECK(mat->GetAxisX().GetBinCount() == 1024);
        CHECK(mat->GetAxisX().GetBinCountAll() == mat->GetAxisX().GetBinCount() + 2);
        CHECK(mat->GetAxisY().GetBinCount() == 2048);
        CHECK(mat->GetAxisY().GetBinCountAll() == mat->GetAxisY().GetBinCount() + 2);
    }

    SUBCASE("Fill and lookup") {
        ts_mat.Fill(83, 283.2);
        CHECK(mat->GetEntries() == 0);

        ts_mat.Fill(83.5, 283.1);
        ts_mat.force_flush();
        CHECK(mat->GetEntries() == 2);
        CHECK(mat->GetBinContent(mat->GetAxisX().FindBin(83.5),
                                 mat->GetAxisY().FindBin(283.15)) == 2);

    }
}

TEST_CASE( "Thread safe 3D histogram" ){

    ThreadSafeHistogram3D ts_cube = histograms.Create3D("cube", "cube title", 1024, 0, 1024, "x", 2048, 0, 2048, "y", 10, 0, 100, "z");
    Histogram3Dp cube = histograms.GetHistograms().Find3D("cube");

    CHECK( cube != nullptr );

    SUBCASE("Metadata") {
        CHECK(cube->GetName() == "cube");
        CHECK(cube->GetTitle() == "cube title");
        CHECK(cube->GetAxisX().GetName() == "cube_xaxis");
        CHECK(cube->GetAxisY().GetName() == "cube_yaxis");
        CHECK(cube->GetAxisZ().GetName() == "cube_zaxis");
        CHECK(cube->GetAxisX().GetTitle() == "x");
        CHECK(cube->GetAxisY().GetTitle() == "y");
        CHECK(cube->GetAxisZ().GetTitle() == "z");

        CHECK(cube->GetAxisX().GetLeft() == 0);
        CHECK(cube->GetAxisX().GetRight() == 1024);
        CHECK(cube->GetAxisX().GetBinWidth() == 1.0);

        CHECK(cube->GetAxisY().GetLeft() == 0);
        CHECK(cube->GetAxisY().GetRight() == 2048);
        CHECK(cube->GetAxisY().GetBinWidth() == 1.0);

        CHECK(cube->GetAxisZ().GetLeft() == 0);
        CHECK(cube->GetAxisZ().GetRight() == 100);
        CHECK(cube->GetAxisZ().GetBinWidth() == 10.0);
    }

    SUBCASE("Number of bins"){
        CHECK(cube->GetAxisX().GetBinCount() == 1024);
        CHECK(cube->GetAxisX().GetBinCountAll() == cube->GetAxisX().GetBinCount() + 2);
        CHECK(cube->GetAxisY().GetBinCount() == 2048);
        CHECK(cube->GetAxisX().GetBinCountAll() == cube->GetAxisX().GetBinCount() + 2);
        CHECK(cube->GetAxisZ().GetBinCount() == 10);
        CHECK(cube->GetAxisX().GetBinCountAll() == cube->GetAxisX().GetBinCount() + 2);
        }

        SUBCASE("Fill and lookup"){
        ts_cube.Fill(83, 283.2, 29);
        CHECK(cube->GetEntries() == 0);

        ts_cube.Fill(83.5, 283.1, 28);
        ts_cube.force_flush();
        CHECK(cube->GetEntries() == 2);
        CHECK(cube->GetBinContent(
                cube->GetAxisX().FindBin(83.5),
                cube->GetAxisY().FindBin(283.15),
                cube->GetAxisZ().FindBin(28.5)) == 2);

    }
}

TEST_SUITE_END();