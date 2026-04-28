#ifndef SHAREDHISTOGRAMS_H_
#define SHAREDHISTOGRAMS_H_

#include <histogram/Histograms.h>

#include <cstdint>
#include <memory>
#include <string>

class SharedHistogram1D;
class SharedHistogram2D;
class SharedHistogram3D;

using SharedHistogram1Dp = std::shared_ptr<SharedHistogram1D>;
using SharedHistogram2Dp = std::shared_ptr<SharedHistogram2D>;
using SharedHistogram3Dp = std::shared_ptr<SharedHistogram3D>;

class SharedHistograms;

class SharedHistogram1D : public Named {
public:
    using data_t = int64_t;

    void Fill(Axis::bin_t x, data_t weight = 1);
    data_t GetBinContent(Axis::index_t bin) const;
    [[nodiscard]] const Axis &GetAxisX() const { return xaxis; }
    [[nodiscard]] std::uint64_t GetEntries() const;
    void Reset();

private:
    friend class SharedHistograms;
    SharedHistogram1D(const std::string &name, const std::string &title, const std::string &path,
                      Axis::index_t channels, Axis::bin_t left, Axis::bin_t right, const std::string &xtitle,
                      std::uint64_t *entries_ptr, data_t *data_ptr, bool writable);

    const Axis xaxis;
    std::uint64_t *entries;
    data_t *data;
    bool writable;
};

class SharedHistogram2D : public Named {
public:
    using data_t = int64_t;

    void Fill(Axis::bin_t x, Axis::bin_t y, data_t weight = 1);
    data_t GetBinContent(Axis::index_t xbin, Axis::index_t ybin) const;
    [[nodiscard]] const Axis &GetAxisX() const { return xaxis; }
    [[nodiscard]] const Axis &GetAxisY() const { return yaxis; }
    [[nodiscard]] std::uint64_t GetEntries() const;
    void Reset();

private:
    friend class SharedHistograms;
    SharedHistogram2D(const std::string &name, const std::string &title, const std::string &path,
                      Axis::index_t xchannels, Axis::bin_t xleft, Axis::bin_t xright, const std::string &xtitle,
                      Axis::index_t ychannels, Axis::bin_t yleft, Axis::bin_t yright, const std::string &ytitle,
                      std::uint64_t *entries_ptr, data_t *data_ptr, bool writable);

    [[nodiscard]] Axis::index_t FlatIndex(Axis::index_t xbin, Axis::index_t ybin) const;

    const Axis xaxis;
    const Axis yaxis;
    std::uint64_t *entries;
    data_t *data;
    bool writable;
};

class SharedHistogram3D : public Named {
public:
    using data_t = int64_t;

    void Fill(Axis::bin_t x, Axis::bin_t y, Axis::bin_t z, data_t weight = 1);
    data_t GetBinContent(Axis::index_t xbin, Axis::index_t ybin, Axis::index_t zbin) const;
    [[nodiscard]] const Axis &GetAxisX() const { return xaxis; }
    [[nodiscard]] const Axis &GetAxisY() const { return yaxis; }
    [[nodiscard]] const Axis &GetAxisZ() const { return zaxis; }
    [[nodiscard]] std::uint64_t GetEntries() const;
    void Reset();

private:
    friend class SharedHistograms;
    SharedHistogram3D(const std::string &name, const std::string &title, const std::string &path,
                      Axis::index_t xchannels, Axis::bin_t xleft, Axis::bin_t xright, const std::string &xtitle,
                      Axis::index_t ychannels, Axis::bin_t yleft, Axis::bin_t yright, const std::string &ytitle,
                      Axis::index_t zchannels, Axis::bin_t zleft, Axis::bin_t zright, const std::string &ztitle,
                      std::uint64_t *entries_ptr, data_t *data_ptr, bool writable);

    [[nodiscard]] Axis::index_t FlatIndex(Axis::index_t xbin, Axis::index_t ybin, Axis::index_t zbin) const;

    const Axis xaxis;
    const Axis yaxis;
    const Axis zaxis;
    std::uint64_t *entries;
    data_t *data;
    bool writable;
};

class SharedHistograms {
public:
    static SharedHistograms Create(const std::string &shared_name, std::size_t shared_memory_size,
                                   std::size_t max_histograms = 256, bool unlink_on_destroy = false);
    static SharedHistograms Attach(const std::string &shared_name, bool read_only = true);

    SharedHistograms(SharedHistograms &&) noexcept;
    SharedHistograms &operator=(SharedHistograms &&) noexcept;
    SharedHistograms(const SharedHistograms &) = delete;
    SharedHistograms &operator=(const SharedHistograms &) = delete;
    ~SharedHistograms();

    SharedHistogram1Dp Create1D(const std::string &name, const std::string &title, Axis::index_t channels,
                                Axis::bin_t left, Axis::bin_t right, const std::string &xtitle,
                                const std::string &path = "");
    SharedHistogram2Dp Create2D(const std::string &name, const std::string &title, Axis::index_t xchannels,
                                Axis::bin_t xleft, Axis::bin_t xright, const std::string &xtitle,
                                Axis::index_t ychannels, Axis::bin_t yleft, Axis::bin_t yright,
                                const std::string &ytitle, const std::string &path = "");
    SharedHistogram3Dp Create3D(const std::string &name, const std::string &title, Axis::index_t xchannels,
                                Axis::bin_t xleft, Axis::bin_t xright, const std::string &xtitle,
                                Axis::index_t ychannels, Axis::bin_t yleft, Axis::bin_t yright,
                                const std::string &ytitle, Axis::index_t zchannels, Axis::bin_t zleft,
                                Axis::bin_t zright, const std::string &ztitle, const std::string &path = "");

    SharedHistogram1Dp Find1D(const std::string &name) const;
    SharedHistogram2Dp Find2D(const std::string &name) const;
    SharedHistogram3Dp Find3D(const std::string &name) const;

private:
    struct Impl;
    explicit SharedHistograms(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl;
};

#endif // SHAREDHISTOGRAMS_H_
