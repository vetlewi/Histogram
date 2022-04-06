//
// Created by Vetle Wegner Ingeberg on 05/04/2022.
//

#include <doctest/doctest.h>
#include <histogram/version.h>
#include <Histograms.h>
#include <Histogram1D.h>
#include <Histogram2D.h>
#include <Histogram3D.h>

#include <iostream>

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
    }

    SUBCASE("Number of bins"){
        CHECK(hist->GetAxisX().GetBinCount() == 1024);
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
    }

    SUBCASE("Over/underflow"){
        hist->Fill(-103020.2);
        CHECK(hist->GetBinContent(0) == 1);

        hist->Fill(929292.1);
        CHECK(hist->GetBinContent(hist->GetAxisX().GetBinCountAll()-1) == 1);
    }

    SUBCASE("Find 1D histogram"){
        auto hist2 = histograms.Find1D("hist");
        CHECK( hist2 != nullptr );
        CHECK(hist2 == hist);

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
    }

    SUBCASE("Number of bins"){
        CHECK(mat->GetAxisX().GetBinCount() == 1024);
        CHECK(mat->GetAxisY().GetBinCount() == 2048);
    }

    SUBCASE("Fill and lookup"){
        mat->Fill(83, 283.2);
        CHECK(mat->GetEntries() == 1);

        mat->Fill(83.5, 283.1);
        CHECK(mat->GetEntries() == 2);
        CHECK(mat->GetBinContent(mat->GetAxisX().FindBin(83.5),
                                 mat->GetAxisY().FindBin(283.15)) == 2);

    }

    SUBCASE("Fill and reset"){
        CHECK(mat->GetEntries() == 0);

        mat->Fill(83, 831.);
        CHECK(mat->GetEntries() == 1);

        mat->Reset();
        CHECK(mat->GetEntries() == 0);
    }

    SUBCASE("Find 2D histogram"){
        auto mat2 = histograms.Find2D("mat");
        CHECK( mat2 != nullptr );
        CHECK(mat2 == mat);

        mat2 = histograms.Find2D("blah");
        CHECK(mat2 == nullptr);
        CHECK(mat2 != mat);

    }

    SUBCASE("Get list of 1D histograms"){
        auto mat2 = histograms.Create2D("mat2", "mat2", 2048, 0, 2048, "x2", 1024, -512, 512, "y2");
        mat2->Fill(93, 21.1);
        for ( auto &h : histograms.GetAll2D() ){
            auto _h = histograms.Find2D(h->GetName());
            CHECK(_h == h);
        }
    }


}

TEST_SUITE_END();