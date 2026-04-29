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
    return "/histogram_test_" + suffix;
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
    auto c4 = consumer.Find3D("h4");

    REQUIRE(c1.get() != nullptr);
    REQUIRE(c2.get() != nullptr);
    REQUIRE(c3.get() != nullptr);
    REQUIRE(c4.get() == nullptr);

    CHECK(c1->GetEntries() == 1);
    CHECK(c2->GetEntries() == 1);
    CHECK(c3->GetEntries() == 1);

    CHECK(c1->GetBinContent(c1->GetAxisX().FindBin(4.2)) == 3);
    CHECK(c2->GetBinContent(c2->GetAxisX().FindBin(1.1), c2->GetAxisY().FindBin(2.2)) == 5);
    CHECK(c3->GetBinContent(c3->GetAxisX().FindBin(1.1), c3->GetAxisY().FindBin(2.2), c3->GetAxisZ().FindBin(3.3)) == 7);
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

TEST_CASE("GetAll1D/2D/3D returns correct histograms")
{
#if defined(__linux__) || defined(__APPLE__)
    const std::string shared_name = MakeSharedName("getall");
    ::shm_unlink(shared_name.c_str());

    auto creator = SharedHistograms::Create(shared_name, 2 * 1024 * 1024, 16, true);

    auto h1 = creator.Create1D("h1", "t1", 10, 0, 10, "x");
    auto h2 = creator.Create1D("h2", "t2", 10, 0, 10, "x");
    auto h3 = creator.Create2D("h3", "t3", 5, 0, 5, "x", 6, 0, 6, "y");
    auto h4 = creator.Create3D("h4", "t4", 3, 0, 3, "x", 4, 0, 4, "y", 5, 0, 5, "z");

    auto consumer = SharedHistograms::Attach(shared_name);

    auto list1 = consumer.GetAll1D();
    auto list2 = consumer.GetAll2D();
    auto list3 = consumer.GetAll3D();

    CHECK(list1.size() == 2);
    CHECK(list2.size() == 1);
    CHECK(list3.size() == 1);

    // Verify names (order not guaranteed, so just check presence)
    bool found_h1 = false, found_h2 = false;
    for (const auto &h : list1) {
        if (h->GetName() == "h1") found_h1 = true;
        if (h->GetName() == "h2") found_h2 = true;
    }

    CHECK(found_h1);
    CHECK(found_h2);
#else
    CHECK(true);
#endif
}

TEST_CASE("ResetAll clears all histograms")
{
#if defined(__linux__) || defined(__APPLE__)
    const std::string shared_name = MakeSharedName("resetall");
    ::shm_unlink(shared_name.c_str());

    auto creator = SharedHistograms::Create(shared_name, 2 * 1024 * 1024, 8, true);

    auto h1 = creator.Create1D("h1", "t1", 10, 0, 10, "x");
    auto h2 = creator.Create2D("h2", "t2", 5, 0, 5, "x", 5, 0, 5, "y");

    h1->Fill(2.0, 5);
    h2->Fill(1.0, 1.0, 7);

    creator.ResetAll();

    CHECK(h1->GetEntries() == 0);
    CHECK(h2->GetEntries() == 0);

    CHECK(h1->GetBinContent(h1->GetAxisX().FindBin(2.0)) == 0);
    CHECK(h2->GetBinContent(
        h2->GetAxisX().FindBin(1.0),
        h2->GetAxisY().FindBin(1.0)
    ) == 0);
#else
    CHECK(true);
#endif
}

TEST_CASE("ResetAll throws on read-only mapping")
{
#if defined(__linux__) || defined(__APPLE__)
    const std::string shared_name = MakeSharedName("reset_ro");
    ::shm_unlink(shared_name.c_str());

    auto creator = SharedHistograms::Create(shared_name, 2 * 1024 * 1024, 8, true);
    creator.Create1D("h1", "t1", 10, 0, 10, "x");

    auto consumer = SharedHistograms::Attach(shared_name, true); // read-only

    CHECK_THROWS(consumer.ResetAll());
#else
    CHECK(true);
#endif
}

TEST_CASE("Stress: many histograms across all dimensions")
{
#if defined(__linux__) || defined(__APPLE__)
    const std::string shared_name = MakeSharedName("stress_many");
    ::shm_unlink(shared_name.c_str());

    constexpr std::size_t N1 = 40;
    constexpr std::size_t N2 = 30;
    constexpr std::size_t N3 = 20;


    auto creator = SharedHistograms::Create(shared_name, 16 * 1024 * 1024, 128, true);

    // Create lots of histograms
    for (std::size_t i = 0; i < N1; ++i) {
        creator.Create1D("h1_" + std::to_string(i), "t", 16, 0, 16, "x");
    }
    for (std::size_t i = 0; i < N2; ++i) {
        creator.Create2D("h2_" + std::to_string(i), "t", 8, 0, 8, "x", 8, 0, 8, "y");
    }
    for (std::size_t i = 0; i < N3; ++i) {
        creator.Create3D("h3_" + std::to_string(i), "t", 4, 0, 4, "x", 4, 0, 4, "y", 4, 0, 4, "z");
    }

    auto consumer = SharedHistograms::Attach(shared_name);

    auto list1 = consumer.GetAll1D();
    auto list2 = consumer.GetAll2D();
    auto list3 = consumer.GetAll3D();

    CHECK(list1.size() == N1);
    CHECK(list2.size() == N2);
    CHECK(list3.size() == N3);

    // Spot-check a few
    CHECK(consumer.Find1D("h1_0") != nullptr);
    CHECK(consumer.Find2D("h2_10") != nullptr);
    CHECK(consumer.Find3D("h3_5") != nullptr);
#else
    CHECK(true);
#endif
}

TEST_CASE("Stress: heavy fill and global reset")
{
#if defined(__linux__) || defined(__APPLE__)
    const std::string shared_name = MakeSharedName("stress2");
    ::shm_unlink(shared_name.c_str());

    constexpr std::size_t N = 20;

    auto creator = SharedHistograms::Create(shared_name, 16 * 1024 * 1024, 64, true);

    std::vector<SharedHistogram1Dp> hists;

    for (std::size_t i = 0; i < N; ++i) {
        auto h = creator.Create1D("h_" + std::to_string(i), "t", 32, 0, 32, "x");
        hists.push_back(h);
    }

    // Fill heavily with different patterns
    for (std::size_t i = 0; i < N; ++i) {
        for (int j = 0; j < 1000; ++j) {
            double value = (j % 32) + 0.5;
            hists[i]->Fill(value, static_cast<int>(i + 1));
        }
    }

    // Verify accumulation
    for (std::size_t i = 0; i < N; ++i) {
        CHECK(hists[i]->GetEntries() == 1000);

        auto bin = hists[i]->GetAxisX().FindBin(5.5);
        CHECK(hists[i]->GetBinContent(bin) > 0); // not exact, just sanity
    }

    // Reset everything
    creator.ResetAll();

    // Verify everything is zeroed
    for (std::size_t i = 0; i < N; ++i) {
        CHECK(hists[i]->GetEntries() == 0);

        for (Axis::index_t b = 0; b < hists[i]->GetAxisX().GetBinCountAll(); ++b) {
            CHECK(hists[i]->GetBinContent(b) == 0);
        }
    }
#else
    CHECK(true);
#endif
}

TEST_SUITE_END();
