#include <doctest/doctest.h>

#include <histogram/SharedHistograms.h>

#include <string>

#if defined(__linux__) || defined(__APPLE__)
#include <sys/mman.h>
#include <unistd.h>
#endif

TEST_SUITE_BEGIN("SharedHistograms");

namespace {

std::string MakeSharedName(const std::string &suffix)
{
#if defined(__linux__) || defined(__APPLE__)
    return "/histogram_test_" + suffix + "_" + std::to_string(static_cast<long long>(::getpid()));
#else
    return "histogram_test_" + suffix;
#endif
}

} // namespace

TEST_CASE("Creator and consumer can share 1D/2D/3D histograms")
{
#if defined(__linux__) || defined(__APPLE__)
    const std::string shared_name = MakeSharedName("basic");
    ::shm_unlink(shared_name.c_str());

    auto creator = SharedHistograms::Create(shared_name, 4 * 1024 * 1024, 16, true);
    auto h1 = creator.Create1D("h1", "title1", 16, 0, 16, "x");
    auto h2 = creator.Create2D("h2", "title2", 8, 0, 8, "x", 10, -5, 5, "y");
    auto h3 = creator.Create3D("h3", "title3", 4, 0, 4, "x", 5, 0, 5, "y", 6, 0, 6, "z");

    h1->Fill(4.2, 3);
    h2->Fill(1.1, 2.2, 5);
    h3->Fill(1.1, 2.2, 3.3, 7);

    auto consumer = SharedHistograms::Attach(shared_name);
    auto c1 = consumer.Find1D("h1");
    auto c2 = consumer.Find2D("h2");
    auto c3 = consumer.Find3D("h3");

    REQUIRE(c1 != nullptr);
    REQUIRE(c2 != nullptr);
    REQUIRE(c3 != nullptr);

    CHECK(c1->GetEntries() == 1);
    CHECK(c2->GetEntries() == 1);
    CHECK(c3->GetEntries() == 1);

    CHECK(c1->GetBinContent(c1->GetAxisX().FindBin(4.2)) == 3);
    CHECK(c2->GetBinContent(c2->GetAxisX().FindBin(1.1), c2->GetAxisY().FindBin(2.2)) == 5);
    CHECK(c3->GetBinContent(c3->GetAxisX().FindBin(1.1), c3->GetAxisY().FindBin(2.2), c3->GetAxisZ().FindBin(3.3)) ==
          7);
#else
    CHECK_THROWS(SharedHistograms::Create("unsupported", 1024, 2, true));
#endif
}

TEST_CASE("Consumer cannot create histograms")
{
#if defined(__linux__) || defined(__APPLE__)
    const std::string shared_name = MakeSharedName("readonly");
    ::shm_unlink(shared_name.c_str());

    auto creator = SharedHistograms::Create(shared_name, 2 * 1024 * 1024, 8, true);
    creator.Create1D("h1", "title1", 16, 0, 16, "x");

    auto consumer = SharedHistograms::Attach(shared_name);
    CHECK_THROWS(consumer.Create1D("h2", "title2", 16, 0, 16, "x"));
#else
    CHECK(true);
#endif
}

TEST_CASE("Shared attach fails on missing segment")
{
    CHECK_THROWS(SharedHistograms::Attach("/histogram_non_existing_segment_123456"));
}

TEST_SUITE_END();
