#include <histogram/SharedHistograms.h>
#include <histogram/internal/SharedMemoryBackend.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace {

constexpr std::uint32_t kSharedMagic = 0x48535447; // HSTG
constexpr std::uint32_t kSharedVersion = 1;
constexpr std::size_t kStringFieldLength = 96;

enum class SharedHistogramDimension : std::uint8_t { D1 = 1, D2 = 2, D3 = 3 };

struct SharedHistogramHeader {
    std::uint32_t magic;
    std::uint32_t version;
    std::uint64_t total_size;
    std::uint64_t max_histograms;
    std::uint64_t used_histograms;
    std::uint64_t next_data_offset;
};

struct SharedHistogramDescriptor {
    std::uint8_t in_use;
    std::uint8_t dimension;
    std::uint8_t reserved[6];

    char name[kStringFieldLength];
    char title[kStringFieldLength];
    char path[kStringFieldLength];
    char xtitle[kStringFieldLength];
    char ytitle[kStringFieldLength];
    char ztitle[kStringFieldLength];

    std::uint64_t xchannels;
    std::uint64_t ychannels;
    std::uint64_t zchannels;

    double xleft;
    double xright;
    double yleft;
    double yright;
    double zleft;
    double zright;

    std::uint64_t entries;
    std::uint64_t data_offset;
    std::uint64_t data_count;
};

static_assert(std::is_standard_layout<SharedHistogramHeader>::value, "Shared header must be standard layout");
static_assert(std::is_standard_layout<SharedHistogramDescriptor>::value, "Shared descriptor must be standard layout");

template <typename T>
T *OffsetPtr(void *base, std::uint64_t offset)
{
    return reinterpret_cast<T *>(reinterpret_cast<std::uint8_t *>(base) + offset);
}

template <typename T>
const T *OffsetPtr(const void *base, std::uint64_t offset)
{
    return reinterpret_cast<const T *>(reinterpret_cast<const std::uint8_t *>(base) + offset);
}

std::size_t RequiredDataCount1D(Axis::index_t xchannels)
{
    return xchannels + 2;
}

std::size_t RequiredDataCount2D(Axis::index_t xchannels, Axis::index_t ychannels)
{
    return (xchannels + 2) * (ychannels + 2);
}

std::size_t RequiredDataCount3D(Axis::index_t xchannels, Axis::index_t ychannels, Axis::index_t zchannels)
{
    return (xchannels + 2) * (ychannels + 2) * (zchannels + 2);
}

void CopyStringToField(std::array<char, kStringFieldLength> &dst, const std::string &src)
{
    dst.fill('\0');
    if (src.empty()) {
        return;
    }
    const std::size_t len = std::min(src.size(), kStringFieldLength - 1);
    std::memcpy(dst.data(), src.data(), len);
}

void CopyStringToField(char dst[kStringFieldLength], const std::string &src)
{
    std::array<char, kStringFieldLength> buffer {};
    CopyStringToField(buffer, src);
    std::memcpy(dst, buffer.data(), kStringFieldLength);
}

std::string ReadField(const char src[kStringFieldLength])
{
    return std::string(src, ::strnlen(src, kStringFieldLength));
}

void EnsureWritable(bool writable)
{
    if (!writable) {
        throw std::runtime_error("Shared histogram is read-only");
    }
}

} // namespace

struct SharedHistograms::Impl {
    HistogramInternal::SharedMemoryHandle memory;
    SharedHistogramHeader *header;
    SharedHistogramDescriptor *descriptors;
    bool creator;
    bool writable;

    explicit Impl(HistogramInternal::SharedMemoryHandle &&handle, bool creator_role, bool writable_mapping)
        : memory(std::move(handle))
        , header(reinterpret_cast<SharedHistogramHeader *>(memory.address))
        , descriptors(OffsetPtr<SharedHistogramDescriptor>(memory.address, sizeof(SharedHistogramHeader)))
        , creator(creator_role)
        , writable(writable_mapping)
    {
    }

    SharedHistogramDescriptor *FindDescriptor(const std::string &name) const
    {
        for (std::size_t i = 0; i < header->max_histograms; ++i) {
            auto *d = &descriptors[i];
            if (d->in_use == 0) {
                continue;
            }
            if (ReadField(d->name) == name) {
                return d;
            }
        }
        return nullptr;
    }

    SharedHistogramDescriptor *CreateDescriptor(SharedHistogramDimension dim, const std::string &name,
                                                const std::string &title, const std::string &path,
                                                const std::string &xtitle, const std::string &ytitle,
                                                const std::string &ztitle, Axis::index_t xchannels,
                                                Axis::bin_t xleft, Axis::bin_t xright, Axis::index_t ychannels,
                                                Axis::bin_t yleft, Axis::bin_t yright, Axis::index_t zchannels,
                                                Axis::bin_t zleft, Axis::bin_t zright, std::size_t data_count)
    {
        if (!creator) {
            throw std::runtime_error("Only creator can define shared histograms");
        }
        EnsureWritable(writable);

        if (FindDescriptor(name) != nullptr) {
            throw std::runtime_error("Histogram with name '" + name + "' already exists in shared memory");
        }

        SharedHistogramDescriptor *slot = nullptr;
        for (std::size_t i = 0; i < header->max_histograms; ++i) {
            if (descriptors[i].in_use == 0) {
                slot = &descriptors[i];
                break;
            }
        }
        if (slot == nullptr) {
            throw std::runtime_error("No free shared histogram descriptors");
        }

        const std::uint64_t bytes = static_cast<std::uint64_t>(data_count * sizeof(std::int64_t));
        if (header->next_data_offset + bytes > header->total_size) {
            throw std::runtime_error("Shared memory exhausted while creating histogram '" + name + "'");
        }

        std::memset(slot, 0, sizeof(SharedHistogramDescriptor));
        slot->in_use = 1;
        slot->dimension = static_cast<std::uint8_t>(dim);
        CopyStringToField(slot->name, name);
        CopyStringToField(slot->title, title);
        CopyStringToField(slot->path, path);
        CopyStringToField(slot->xtitle, xtitle);
        CopyStringToField(slot->ytitle, ytitle);
        CopyStringToField(slot->ztitle, ztitle);
        slot->xchannels = xchannels;
        slot->ychannels = ychannels;
        slot->zchannels = zchannels;
        slot->xleft = xleft;
        slot->xright = xright;
        slot->yleft = yleft;
        slot->yright = yright;
        slot->zleft = zleft;
        slot->zright = zright;
        slot->entries = 0;
        slot->data_offset = header->next_data_offset;
        slot->data_count = data_count;

        auto *data = OffsetPtr<std::int64_t>(memory.address, slot->data_offset);
        std::fill(data, data + data_count, 0);

        header->next_data_offset += bytes;
        header->used_histograms += 1;
        return slot;
    }

    std::int64_t *GetData(const SharedHistogramDescriptor *d) const
    {
        return OffsetPtr<std::int64_t>(memory.address, d->data_offset);
    }
};

SharedHistogram1D::SharedHistogram1D(const std::string &name, const std::string &title, const std::string &path,
                                     Axis::index_t channels, Axis::bin_t left, Axis::bin_t right,
                                     const std::string &xtitle, std::uint64_t *entries_ptr, data_t *data_ptr,
                                     bool is_writable)
    : Named(name, title, path)
    , xaxis(name + "_xaxis", channels, left, right, xtitle)
    , entries(entries_ptr)
    , data(data_ptr)
    , writable(is_writable)
{
}

void SharedHistogram1D::Fill(Axis::bin_t x, data_t weight)
{
    EnsureWritable(writable);
    *entries += 1;
    data[xaxis.FindBin(x)] += weight;
}

SharedHistogram1D::data_t SharedHistogram1D::GetBinContent(Axis::index_t bin) const
{
    if (bin >= xaxis.GetBinCountAll()) {
        return 0;
    }
    return data[bin];
}

std::uint64_t SharedHistogram1D::GetEntries() const { return *entries; }

void SharedHistogram1D::Reset()
{
    EnsureWritable(writable);
    for (Axis::index_t i = 0; i < xaxis.GetBinCountAll(); ++i) {
        data[i] = 0;
    }
    *entries = 0;
}

SharedHistogram2D::SharedHistogram2D(const std::string &name, const std::string &title, const std::string &path,
                                     Axis::index_t xchannels, Axis::bin_t xleft, Axis::bin_t xright,
                                     const std::string &xtitle, Axis::index_t ychannels, Axis::bin_t yleft,
                                     Axis::bin_t yright, const std::string &ytitle, std::uint64_t *entries_ptr,
                                     data_t *data_ptr, bool is_writable)
    : Named(name, title, path)
    , xaxis(name + "_xaxis", xchannels, xleft, xright, xtitle)
    , yaxis(name + "_yaxis", ychannels, yleft, yright, ytitle)
    , entries(entries_ptr)
    , data(data_ptr)
    , writable(is_writable)
{
}

Axis::index_t SharedHistogram2D::FlatIndex(Axis::index_t xbin, Axis::index_t ybin) const
{
    return xaxis.GetBinCountAll() * ybin + xbin;
}

void SharedHistogram2D::Fill(Axis::bin_t x, Axis::bin_t y, data_t weight)
{
    EnsureWritable(writable);
    const auto xbin = xaxis.FindBin(x);
    const auto ybin = yaxis.FindBin(y);
    data[FlatIndex(xbin, ybin)] += weight;
    *entries += 1;
}

SharedHistogram2D::data_t SharedHistogram2D::GetBinContent(Axis::index_t xbin, Axis::index_t ybin) const
{
    if (xbin >= xaxis.GetBinCountAll() || ybin >= yaxis.GetBinCountAll()) {
        return 0;
    }
    return data[FlatIndex(xbin, ybin)];
}

std::uint64_t SharedHistogram2D::GetEntries() const { return *entries; }

void SharedHistogram2D::Reset()
{
    EnsureWritable(writable);
    const auto yall = yaxis.GetBinCountAll();
    const auto xall = xaxis.GetBinCountAll();
    for (Axis::index_t y = 0; y < yall; ++y) {
        for (Axis::index_t x = 0; x < xall; ++x) {
            data[FlatIndex(x, y)] = 0;
        }
    }
    *entries = 0;
}

SharedHistogram3D::SharedHistogram3D(const std::string &name, const std::string &title, const std::string &path,
                                     Axis::index_t xchannels, Axis::bin_t xleft, Axis::bin_t xright,
                                     const std::string &xtitle, Axis::index_t ychannels, Axis::bin_t yleft,
                                     Axis::bin_t yright, const std::string &ytitle, Axis::index_t zchannels,
                                     Axis::bin_t zleft, Axis::bin_t zright, const std::string &ztitle,
                                     std::uint64_t *entries_ptr, data_t *data_ptr, bool is_writable)
    : Named(name, title, path)
    , xaxis(name + "_xaxis", xchannels, xleft, xright, xtitle)
    , yaxis(name + "_yaxis", ychannels, yleft, yright, ytitle)
    , zaxis(name + "_zaxis", zchannels, zleft, zright, ztitle)
    , entries(entries_ptr)
    , data(data_ptr)
    , writable(is_writable)
{
}

Axis::index_t SharedHistogram3D::FlatIndex(Axis::index_t xbin, Axis::index_t ybin, Axis::index_t zbin) const
{
    return xaxis.GetBinCountAll() * yaxis.GetBinCountAll() * zbin + xaxis.GetBinCountAll() * ybin + xbin;
}

void SharedHistogram3D::Fill(Axis::bin_t x, Axis::bin_t y, Axis::bin_t z, data_t weight)
{
    EnsureWritable(writable);
    const auto xbin = xaxis.FindBin(x);
    const auto ybin = yaxis.FindBin(y);
    const auto zbin = zaxis.FindBin(z);
    data[FlatIndex(xbin, ybin, zbin)] += weight;
    *entries += 1;
}

SharedHistogram3D::data_t SharedHistogram3D::GetBinContent(Axis::index_t xbin, Axis::index_t ybin,
                                                           Axis::index_t zbin) const
{
    if (xbin >= xaxis.GetBinCountAll() || ybin >= yaxis.GetBinCountAll() || zbin >= zaxis.GetBinCountAll()) {
        return 0;
    }
    return data[FlatIndex(xbin, ybin, zbin)];
}

std::uint64_t SharedHistogram3D::GetEntries() const { return *entries; }

void SharedHistogram3D::Reset()
{
    EnsureWritable(writable);
    const auto xall = xaxis.GetBinCountAll();
    const auto yall = yaxis.GetBinCountAll();
    const auto zall = zaxis.GetBinCountAll();
    for (Axis::index_t z = 0; z < zall; ++z) {
        for (Axis::index_t y = 0; y < yall; ++y) {
            for (Axis::index_t x = 0; x < xall; ++x) {
                data[FlatIndex(x, y, z)] = 0;
            }
        }
    }
    *entries = 0;
}

SharedHistograms::SharedHistograms(std::unique_ptr<Impl> pimpl)
    : impl(std::move(pimpl))
{
}

SharedHistograms::~SharedHistograms() = default;
SharedHistograms::SharedHistograms(SharedHistograms &&) noexcept = default;
SharedHistograms &SharedHistograms::operator=(SharedHistograms &&) noexcept = default;

SharedHistograms SharedHistograms::Create(const std::string &shared_name, std::size_t shared_memory_size,
                                          std::size_t max_histograms, bool unlink_on_destroy)
{
    if (max_histograms == 0) {
        throw std::runtime_error("max_histograms must be larger than 0");
    }
    const std::size_t minimum_size =
            sizeof(SharedHistogramHeader) + max_histograms * sizeof(SharedHistogramDescriptor) + sizeof(std::int64_t);
    if (shared_memory_size < minimum_size) {
        throw std::runtime_error("shared_memory_size is too small for header and descriptor table");
    }

    auto mem = HistogramInternal::CreateSharedMemory(shared_name, shared_memory_size, unlink_on_destroy);
    auto *header = reinterpret_cast<SharedHistogramHeader *>(mem.address);
    std::memset(mem.address, 0, shared_memory_size);
    header->magic = kSharedMagic;
    header->version = kSharedVersion;
    header->total_size = shared_memory_size;
    header->max_histograms = max_histograms;
    header->used_histograms = 0;
    header->next_data_offset = sizeof(SharedHistogramHeader) + max_histograms * sizeof(SharedHistogramDescriptor);

    return SharedHistograms(std::make_unique<Impl>(std::move(mem), true, true));
}

SharedHistograms SharedHistograms::Attach(const std::string &shared_name, bool read_only)
{
    auto mem = HistogramInternal::AttachSharedMemory(shared_name, read_only);
    auto *header = reinterpret_cast<SharedHistogramHeader *>(mem.address);
    if (header->magic != kSharedMagic) {
        throw std::runtime_error("Shared memory segment has invalid histogram magic");
    }
    if (header->version != kSharedVersion) {
        throw std::runtime_error("Shared memory segment has incompatible version");
    }
    return SharedHistograms(std::make_unique<Impl>(std::move(mem), false, !read_only));
}

SharedHistogram1Dp SharedHistograms::Create1D(const std::string &name, const std::string &title,
                                              Axis::index_t channels, Axis::bin_t left, Axis::bin_t right,
                                              const std::string &xtitle, const std::string &path)
{
    auto *d = impl->CreateDescriptor(SharedHistogramDimension::D1, name, title, path, xtitle, "", "", channels, left,
                                     right, 0, 0, 0, 0, 0, 0, RequiredDataCount1D(channels));
    auto *data = impl->GetData(d);
    return SharedHistogram1Dp(new SharedHistogram1D(ReadField(d->name), ReadField(d->title), ReadField(d->path),
                                                    d->xchannels, d->xleft, d->xright, ReadField(d->xtitle),
                                                    &d->entries, data, impl->writable));
}

SharedHistogram2Dp SharedHistograms::Create2D(const std::string &name, const std::string &title, Axis::index_t xchannels,
                                              Axis::bin_t xleft, Axis::bin_t xright, const std::string &xtitle,
                                              Axis::index_t ychannels, Axis::bin_t yleft, Axis::bin_t yright,
                                              const std::string &ytitle, const std::string &path)
{
    auto *d = impl->CreateDescriptor(SharedHistogramDimension::D2, name, title, path, xtitle, ytitle, "", xchannels,
                                     xleft, xright, ychannels, yleft, yright, 0, 0, 0,
                                     RequiredDataCount2D(xchannels, ychannels));
    auto *data = impl->GetData(d);
    return SharedHistogram2Dp(new SharedHistogram2D(ReadField(d->name), ReadField(d->title), ReadField(d->path),
                                                    d->xchannels, d->xleft, d->xright, ReadField(d->xtitle),
                                                    d->ychannels, d->yleft, d->yright, ReadField(d->ytitle),
                                                    &d->entries, data, impl->writable));
}

SharedHistogram3Dp SharedHistograms::Create3D(const std::string &name, const std::string &title, Axis::index_t xchannels,
                                              Axis::bin_t xleft, Axis::bin_t xright, const std::string &xtitle,
                                              Axis::index_t ychannels, Axis::bin_t yleft, Axis::bin_t yright,
                                              const std::string &ytitle, Axis::index_t zchannels, Axis::bin_t zleft,
                                              Axis::bin_t zright, const std::string &ztitle, const std::string &path)
{
    auto *d = impl->CreateDescriptor(SharedHistogramDimension::D3, name, title, path, xtitle, ytitle, ztitle,
                                     xchannels, xleft, xright, ychannels, yleft, yright, zchannels, zleft, zright,
                                     RequiredDataCount3D(xchannels, ychannels, zchannels));
    auto *data = impl->GetData(d);
    return SharedHistogram3Dp(new SharedHistogram3D(
            ReadField(d->name), ReadField(d->title), ReadField(d->path), d->xchannels, d->xleft, d->xright,
            ReadField(d->xtitle), d->ychannels, d->yleft, d->yright, ReadField(d->ytitle), d->zchannels, d->zleft,
            d->zright, ReadField(d->ztitle), &d->entries, data, impl->writable));
}

SharedHistogram1Dp SharedHistograms::Find1D(const std::string &name) const
{
    auto *d = impl->FindDescriptor(name);
    if (d == nullptr || d->dimension != static_cast<std::uint8_t>(SharedHistogramDimension::D1)) {
        return nullptr;
    }
    auto *data = impl->GetData(d);
    return SharedHistogram1Dp(new SharedHistogram1D(ReadField(d->name), ReadField(d->title), ReadField(d->path),
                                                    d->xchannels, d->xleft, d->xright, ReadField(d->xtitle),
                                                    &d->entries, data, impl->writable));
}

SharedHistogram2Dp SharedHistograms::Find2D(const std::string &name) const
{
    auto *d = impl->FindDescriptor(name);
    if (d == nullptr || d->dimension != static_cast<std::uint8_t>(SharedHistogramDimension::D2)) {
        return nullptr;
    }
    auto *data = impl->GetData(d);
    return SharedHistogram2Dp(new SharedHistogram2D(ReadField(d->name), ReadField(d->title), ReadField(d->path),
                                                    d->xchannels, d->xleft, d->xright, ReadField(d->xtitle),
                                                    d->ychannels, d->yleft, d->yright, ReadField(d->ytitle),
                                                    &d->entries, data, impl->writable));
}

SharedHistogram3Dp SharedHistograms::Find3D(const std::string &name) const
{
    auto *d = impl->FindDescriptor(name);
    if (d == nullptr || d->dimension != static_cast<std::uint8_t>(SharedHistogramDimension::D3)) {
        return nullptr;
    }
    auto *data = impl->GetData(d);
    return SharedHistogram3Dp(new SharedHistogram3D(
            ReadField(d->name), ReadField(d->title), ReadField(d->path), d->xchannels, d->xleft, d->xright,
            ReadField(d->xtitle), d->ychannels, d->yleft, d->yright, ReadField(d->ytitle), d->zchannels, d->zleft,
            d->zright, ReadField(d->ztitle), &d->entries, data, impl->writable));
}

SharedHistograms::list1d_t SharedHistograms::GetAll1D() {
    list1d_t result;

    for (std::size_t i = 0; i < impl->header->max_histograms; ++i) {
        auto *d = &impl->descriptors[i];

        if (d->in_use == 0) {
            continue;
        }
        if (d->dimension != static_cast<std::uint8_t>(SharedHistogramDimension::D1)) {
            continue;
        }

        auto *data = impl->GetData(d);

        result.emplace_back(new SharedHistogram1D(
            ReadField(d->name),
            ReadField(d->title),
            ReadField(d->path),
            d->xchannels,
            d->xleft,
            d->xright,
            ReadField(d->xtitle),
            &d->entries,
            data,
            impl->writable
        ));
    }

    return result;
}

SharedHistograms::list2d_t SharedHistograms::GetAll2D() {
    list2d_t result;

    for (std::size_t i = 0; i < impl->header->max_histograms; ++i) {
        auto *d = &impl->descriptors[i];

        if (d->in_use == 0) {
            continue;
        }
        if (d->dimension != static_cast<std::uint8_t>(SharedHistogramDimension::D2)) {
            continue;
        }

        auto *data = impl->GetData(d);

        result.emplace_back(new SharedHistogram2D(
            ReadField(d->name),
            ReadField(d->title),
            ReadField(d->path),
            d->xchannels,
            d->xleft,
            d->xright,
            ReadField(d->xtitle),
            d->ychannels,
            d->yleft,
            d->yright,
            ReadField(d->ytitle),
            &d->entries,
            data,
            impl->writable
        ));
    }

    return result;
}

SharedHistograms::list3d_t SharedHistograms::GetAll3D() {
    list3d_t result;

    for (std::size_t i = 0; i < impl->header->max_histograms; ++i) {
        auto *d = &impl->descriptors[i];

        if (d->in_use == 0) {
            continue;
        }
        if (d->dimension != static_cast<std::uint8_t>(SharedHistogramDimension::D3)) {
            continue;
        }

        auto *data = impl->GetData(d);

        result.emplace_back(new SharedHistogram3D(
            ReadField(d->name),
            ReadField(d->title),
            ReadField(d->path),
            d->xchannels,
            d->xleft,
            d->xright,
            ReadField(d->xtitle),
            d->ychannels,
            d->yleft,
            d->yright,
            ReadField(d->ytitle),
            d->zchannels,
            d->zleft,
            d->zright,
            ReadField(d->ztitle),
            &d->entries,
            data,
            impl->writable
        ));
    }

    return result;
}

void SharedHistograms::ResetAll()
{
    EnsureWritable(impl->writable);

    for (std::size_t i = 0; i < impl->header->max_histograms; ++i) {
        auto *d = &impl->descriptors[i];

        if (d->in_use == 0) {
            continue;
        }

        auto *data = impl->GetData(d);

        // Zero data
        std::fill(data, data + d->data_count, 0);

        // Reset entries
        d->entries = 0;
    }
}