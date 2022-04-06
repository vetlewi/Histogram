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
// Created by Vetle Wegner Ingeberg on 05/04/2022.
//

#include <doctest/doctest.h>
#include <histogram/version.h>
#include <histogram/Histograms.h>
#include <histogram/Histogram1D.h>
#include <histogram/Histogram2D.h>
#include <histogram/Histogram3D.h>
#include <MamaWriter.h>

#include <iostream>
#include <sstream>

TEST_CASE("Histogram version"){
    static_assert(std::string_view(HISTOGRAM_VERSION) == std::string_view("1.0"));
    CHECK(std::string(HISTOGRAM_VERSION) == std::string("1.0"));
}

TEST_SUITE_BEGIN( "Histograms" );

Histograms histograms;
Histogram1Dp hist;
Histogram2Dp mat;
Histogram3Dp cube;

TEST_CASE( "1D histogram" ){

    hist = histograms.Create1D("hist", "hist title", 1024, 0, 1024, "x");
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
        hist->Fill(83);
        CHECK(hist->GetEntries() == 1);

        hist->Fill(83.5);
        CHECK(hist->GetEntries() == 2);
        CHECK(hist->GetBinContent(hist->GetAxisX().FindBin(83.5)) == 2);
    }

    SUBCASE("Fill and reset"){
        CHECK(hist->GetEntries() == 0);

        hist->Fill(83);
        CHECK(hist->GetEntries() == 1);

        hist->Reset();
        CHECK(hist->GetEntries() == 0);

        hist->Fill(83);
        hist->Fill(83.5);
        CHECK(hist->GetEntries() == 2);
    }

    SUBCASE("Add"){
        hist->Fill(32.1);
        hist->Fill(45.1);

        auto hist2 = histograms.Create1D("add", "add", 1024, 0, 1024, "x");
        hist2->Fill(93.1);
        hist2->Fill(1001.);

        hist->Add(hist2, 1.0);
        CHECK(hist->GetBinContent(hist->GetAxisX().FindBin(93.1)) != 0);

        CHECK(hist->GetBinContent(hist->GetAxisX().FindBin(93.1)) ==
              hist2->GetBinContent(hist2->GetAxisX().FindBin(93.1)));

        CHECK(hist->GetBinContent(hist->GetAxisX().FindBin(1001.)) ==
              hist2->GetBinContent(hist2->GetAxisX().FindBin(1001.)));

    }

    SUBCASE("Over/underflow"){
        hist->Fill(-103020.2);
        CHECK(hist->GetBinContent(0) == 1);

        hist->Fill(929292.1);
        CHECK(hist->GetBinContent(hist->GetAxisX().GetBinCountAll()-1) == 1);

        CHECK(hist->GetBinContent(200000) == 0);
    }

    SUBCASE("Find 1D histogram"){
        auto hist2 = histograms.Find1D("hist");
        CHECK( hist2 != nullptr );
        CHECK(hist2 == hist);

        hist2->Fill(293., 192.);
        CHECK(hist->GetEntries() == hist2->GetEntries());

        hist2 = histograms.Find1D("blah");
        CHECK(hist2 == nullptr);
        CHECK(hist2 != hist);

    }

    SUBCASE("Get list of 1D histograms"){
        auto hist2 = histograms.Create1D("hist2", "hist2", 2048, 0, 2048, "x2");
        hist2->Fill(93);
        for ( auto &h : histograms.GetAll1D() ){
            auto _h = histograms.Find1D(h->GetName());
            CHECK(_h == h);
        }
    }
}

TEST_CASE( "2D histogram" ){

    mat = histograms.Create2D("mat", "mat title", 1024, 0, 1024, "x", 2048, 0, 2048, "y");
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

    SUBCASE("Number of bins"){
        CHECK(mat->GetAxisX().GetBinCount() == 1024);
        CHECK(mat->GetAxisX().GetBinCountAll() == mat->GetAxisX().GetBinCount() + 2);
        CHECK(mat->GetAxisY().GetBinCount() == 2048);
        CHECK(mat->GetAxisY().GetBinCountAll() == mat->GetAxisY().GetBinCount() + 2);
    }

    SUBCASE("Fill and lookup"){
        mat->Fill(83, 283.2);
        CHECK(mat->GetEntries() == 1);

        mat->Fill(83.5, 283.1);
        CHECK(mat->GetEntries() == 2);
        CHECK(mat->GetBinContent(mat->GetAxisX().FindBin(83.5),
                                 mat->GetAxisY().FindBin(283.15)) == 2);

    }

    SUBCASE("Add"){
        mat->Fill(32.1, 102.);
        mat->Fill(45.1, 232.);

        auto mat2 = histograms.Create2D("add", "add", 1024, 0, 1024, "x", 2048, 0, 2048, "y");
        mat2->Fill(93.1, 1003);
        mat2->Fill(1001., 1003.1);

        mat->Add(mat2, 1.0);
        CHECK(mat->GetBinContent(mat->GetAxisX().FindBin(93.1),
                                 mat->GetAxisY().FindBin(1003)) != 0);

        CHECK(mat->GetBinContent(mat->GetAxisX().FindBin(93.1),
                                 mat->GetAxisY().FindBin(1003)) ==
              mat2->GetBinContent(mat2->GetAxisX().FindBin(93.1),
                                  mat2->GetAxisX().FindBin(1003)));

        CHECK(mat->GetBinContent(mat->GetAxisX().FindBin(1001.),
                                 mat->GetAxisY().FindBin(1003)) ==
              mat2->GetBinContent(mat2->GetAxisX().FindBin(1001.),
                                  mat2->GetAxisX().FindBin(1003)));

    }

    SUBCASE("Fill and reset"){
        CHECK(mat->GetEntries() == 0);

        mat->Fill(83, 831.);
        CHECK(mat->GetEntries() == 1);
        CHECK(mat->GetBinContent(20000, 3020010) == 0);

        mat->Reset();
        CHECK(mat->GetEntries() == 0);
    }

    SUBCASE("Find 2D histogram"){
        auto mat2 = histograms.Find2D("mat");
        CHECK( mat2 != nullptr );
        CHECK(mat2 == mat);

        mat2->Fill(293., 192.);
        CHECK(mat->GetEntries() == mat2->GetEntries());

        mat2 = histograms.Find2D("blah");
        CHECK(mat2 == nullptr);
        CHECK(mat2 != mat);

    }

    SUBCASE("Get list of 2D histograms"){
        auto mat2 = histograms.Create2D("mat2", "mat2", 2048, 0, 2048, "x2", 1024, -512, 512, "y2");
        mat2->Fill(93, 21.1);
        for ( auto &h : histograms.GetAll2D() ){
            auto _h = histograms.Find2D(h->GetName());
            CHECK(_h == h);
        }
    }
}

TEST_CASE( "3D histogram" ){

    cube = histograms.Create3D("cube", "cube title", 1024, 0, 1024, "x", 2048, 0, 2048, "y", 10, 0, 100, "z");
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
        cube->Fill(83, 283.2, 29);
        CHECK(cube->GetEntries() == 1);

        cube->Fill(83.5, 283.1, 28);
        CHECK(cube->GetEntries() == 2);
        CHECK(cube->GetBinContent(cube->GetAxisX().FindBin(83.5),
                                  cube->GetAxisY().FindBin(283.15),
                                  cube->GetAxisZ().FindBin(28.5)) == 2);

    }

    SUBCASE("Fill and reset"){
        CHECK(cube->GetEntries() == 0);

        cube->Fill(83, 831., 28.1);
        CHECK(cube->GetEntries() == 1);

        cube->Reset();
        CHECK(cube->GetEntries() == 0);
    }

    SUBCASE("Add"){
        cube->Fill(32.1, 102., 2.);
        cube->Fill(45.1, 232., 3.);

        auto cube2 = histograms.Create3D("add", "add", 1024, 0, 1024, "x", 2048, 0, 2048, "y", 10, 0, 100, "z");
        cube2->Fill(93.1, 1003, 81.);
        cube2->Fill(1001., 1003.1, 93.);

        cube->Add(cube2, 1.0);

        CHECK(cube->GetBinContent(cube->GetAxisX().FindBin(93.1),
                                  cube->GetAxisY().FindBin(1003),
                                  cube->GetAxisZ().FindBin(81.)) != 0);

        CHECK(cube->GetBinContent(cube->GetAxisX().FindBin(93.1),
                                  cube->GetAxisY().FindBin(1003),
                                  cube->GetAxisZ().FindBin(81.)) ==
              cube2->GetBinContent(cube2->GetAxisX().FindBin(93.1),
                                   cube2->GetAxisX().FindBin(1003),
                                   cube2->GetAxisZ().FindBin(81.)));

        CHECK(cube->GetBinContent(cube->GetAxisX().FindBin(1001.),
                                  cube->GetAxisY().FindBin(1003.1),
                                  cube->GetAxisZ().FindBin(93.)) ==
              cube2->GetBinContent(cube2->GetAxisX().FindBin(1001.),
                                   cube2->GetAxisX().FindBin(1003.1),
                                   cube2->GetAxisZ().FindBin(93.)));

    }

    SUBCASE("Find 3D histogram"){
        auto cube2 = histograms.Find3D("cube");
        CHECK( cube2 != nullptr );
        CHECK(cube2 == cube);

        cube2->Fill(293., 192., 93.1);
        CHECK(cube->GetEntries() == cube2->GetEntries());

        cube2 = histograms.Find3D("blah");
        CHECK(cube2 == nullptr);
        CHECK(cube2 != cube);

    }

    SUBCASE("Get list of 3D histograms"){
        auto cube2 = histograms.Create3D("cube2", "cube2", 2048, 0, 2048, "x2", 1024, -512, 512, "y2", 10, 0, 100, "z2");
        cube2->Fill(93, 21.1, 31.1);
        for ( auto &h : histograms.GetAll3D() ){
            auto _h = histograms.Find3D(h->GetName());
            CHECK(_h == h);
        }
    }
}

TEST_CASE("Write to MaMa files"){

    // We expect there to be two of each type of histogram
    if ( histograms.GetAll1D().size() == 0 ){
        histograms.Create1D("hist", "hist", 193, 0, 832.1, "x");
        histograms.Create1D("hist2", "hist2", 13, 0, 832.1, "x");
    }
    if ( histograms.GetAll2D().size() == 0 ){
        histograms.Create2D("mat", "mat", 193, 0, 832.1, "x", 192, -10.2, 382.1, "y");
        histograms.Create2D("mat2", "mat2", 13, 0, 832.1, "x", 192, -1.2, 382.1, "y");
    }
    if ( histograms.GetAll3D().size() == 0 ){
        histograms.Create3D("cube", "cube", 193, 0, 832.1, "x", 192, -10.2, 382.1, "y", 10, -2, 3., "z");
        histograms.Create3D("cube2", "cube2", 13, 0, 832.1, "x", 192, -1.2, 382.1, "y", 7, -3., 1., "z");
    }
    REQUIRE(histograms.GetAll1D().size() > 0);
    REQUIRE(histograms.GetAll2D().size() > 0);
    REQUIRE(histograms.GetAll3D().size() > 0);

    SUBCASE("1D histograms") {
        // Write all 1D histograms
        for (auto &h: histograms.GetAll1D()) {
            std::stringstream str;
            CHECK(str.str().size() == 0);
            CHECK(h != nullptr);
            CHECK(MamaWriter::Write(str, h) == 0);
            CHECK(str.str().size() > 0);
        }
    }

    SUBCASE("2D histograms") {
        // Write all 2D histograms
        for (auto &h: histograms.GetAll2D()) {
            std::stringstream str;
            CHECK(str.str().size() == 0);
            CHECK(h != nullptr);
            CHECK(MamaWriter::Write(str, h) == 0);
            CHECK(str.str().size() > 0);
        }
    }

    SUBCASE("3D histograms") {
        // Write all 3D histograms - this will throw
        for (auto &h: histograms.GetAll3D()) {
            std::stringstream str;
            CHECK(str.str().size() == 0);
            CHECK(h != nullptr);
            CHECK_THROWS(MamaWriter::Write(str, h));
            CHECK(str.str().size() == 0);
        }
    }

    // We expect there to be two of each type of histogram
    REQUIRE(histograms.GetAll1D().size() > 0);
    REQUIRE(histograms.GetAll2D().size() > 0);
    REQUIRE(histograms.GetAll3D().size() > 0);

}

TEST_CASE("Histograms"){

    // We expect there to be two of each type of histogram
    if ( histograms.GetAll1D().size() == 0 ){
        histograms.Create1D("hist", "hist", 193, 0, 832.1, "x");
        histograms.Create1D("hist2", "hist2", 13, 0, 832.1, "x");
    }
    if ( histograms.GetAll2D().size() == 0 ){
        histograms.Create2D("mat", "mat", 193, 0, 832.1, "x", 192, -10.2, 382.1, "y");
        histograms.Create2D("mat2", "mat2", 13, 0, 832.1, "x", 192, -1.2, 382.1, "y");
    }
    if ( histograms.GetAll3D().size() == 0 ){
        histograms.Create3D("cube", "cube", 193, 0, 832.1, "x", 192, -10.2, 382.1, "y", 10, -2, 3., "z");
        histograms.Create3D("cube2", "cube2", 13, 0, 832.1, "x", 192, -1.2, 382.1, "y", 7, -3., 1., "z");
    }
    REQUIRE(histograms.GetAll1D().size() > 0);
    REQUIRE(histograms.GetAll2D().size() > 0);
    REQUIRE(histograms.GetAll3D().size() > 0);

    // None of the should be empty
    for ( auto &h : histograms.GetAll1D() ){
        h->Fill(182.);
        CHECK(h->GetEntries() > 0);
    }
    for ( auto &h : histograms.GetAll2D() ){
        h->Fill(182., 281.);
        CHECK(h->GetEntries() > 0);
    }
    for ( auto &h : histograms.GetAll3D() ){
        h->Fill(182., 281., 1.2);
        CHECK(h->GetEntries() > 0);
    }

    // Test merging
    Histograms histograms2;
    auto hist2 = histograms2.Create1D("hist", "hist", 193, 0, 832.1, "x");
    REQUIRE(hist2 != nullptr);
    hist2->Fill(252.);
    histograms2.Create1D("hist3", "hist3", 13, 0, 832.1, "x");
    histograms2.Create2D("mat", "mat", 193, 0, 832.1, "x", 192, -10.2, 382.1, "y")->Fill(252, -1.2);
    histograms2.Create2D("mat3", "mat3", 13, 0, 832.1, "x", 192, -1.2, 382.1, "y");
    histograms2.Create3D("cube", "cube", 193, 0, 832.1, "x", 192, -10.2, 382.1, "y", 10, -2, 3., "z");
    histograms2.Create3D("cube3", "cube3", 13, 0, 832.1, "x", 192, -1.2, 382.1, "y", 7, -3., 1., "z");

    SUBCASE("Merge"){
        hist = histograms.Find1D("hist");
        auto old_entries = hist->GetEntries();
        histograms.Merge(histograms2);
        CHECK(hist->GetEntries() == old_entries + hist2->GetEntries());
    }

    SUBCASE("ResetAll") {
        histograms.ResetAll();

        // None of the should be empty
        for (auto &h: histograms.GetAll1D()) {
            CHECK(h->GetEntries() == 0);
        }
        for (auto &h: histograms.GetAll2D()) {
            CHECK(h->GetEntries() == 0);
        }
        for (auto &h: histograms.GetAll3D()) {
            CHECK(h->GetEntries() == 0);
        }
    }

}

TEST_SUITE_END();