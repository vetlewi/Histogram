#ifndef HISTOGRAM_INTERNAL_SHAREDMEMORYBACKEND_H
#define HISTOGRAM_INTERNAL_SHAREDMEMORYBACKEND_H

#include <cstddef>
#include <string>

namespace HistogramInternal {

struct SharedMemoryHandle {
    int fd;
    void *address;
    std::size_t size;
    std::string name;
    bool owner;
    bool unlink_on_destroy;

    SharedMemoryHandle();
    SharedMemoryHandle(const SharedMemoryHandle &) = delete;
    SharedMemoryHandle &operator=(const SharedMemoryHandle &) = delete;
    SharedMemoryHandle(SharedMemoryHandle &&other) noexcept;
    SharedMemoryHandle &operator=(SharedMemoryHandle &&other) noexcept;
    ~SharedMemoryHandle();
};

SharedMemoryHandle CreateSharedMemory(const std::string &name, std::size_t size, bool unlink_on_destroy);
SharedMemoryHandle AttachSharedMemory(const std::string &name, bool read_only);

} // namespace HistogramInternal

#endif // HISTOGRAM_INTERNAL_SHAREDMEMORYBACKEND_H
