//
// Created by Vetle Wegner Ingeberg on 05/04/2022.
//

#include <doctest/doctest.h>
#include <Histogram1D.h>

TEST_SUITE("Histogram1D"){

    Histogram1D hist("test", "test", 1024, 0, 1024, "x");

    // We will fill 4 values and check that we have indeed 8 values after filling.
    TEST_CASE( "Fill" )
    {
        hist.Fill(89);
        hist.Fill(72);
        hist.Fill(21);
        hist.Fill(34);

        CHECK(hist.GetEntries() == 4);
        hist.Fill(89);
        CHECK(hist.GetEntries() == 5);
        CHECK(hist.GetBinContent(hist.GetAxisX().FindBin(89)) == 2);
    }

    TEST_CASE( "Reset" )
    {
        hist.Reset();
        CHECK(hist.GetEntries() == 0);
    }


    TEST_CASE( "Lookup" )
    {
        hist.Fill(89);
        hist.Fill(89);
        CHECK(hist.GetBinContent(hist.GetAxisX().FindBin(89)) == 2);
        CHECK(hist.GetEntries() == 2);
    }

}
