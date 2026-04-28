#include <histogram/internal/SharedMemoryBackend.h>

#include <stdexcept>

#if defined(__linux__) || defined(__APPLE__)

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <utility>

namespace HistogramInternal {

namespace {

std::string EnsureSharedMemoryName(const std::string &name)
{
    if (name.empty()) {
        throw std::runtime_error("Shared memory name cannot be empty");
    }
    if (name[0] == '/') {
        return name;
    }
    return "/" + name;
}

[[noreturn]] void ThrowErrno(const std::string &prefix)
{
    throw std::runtime_error(prefix + ": " + std::strerror(errno));
}

} // namespace

SharedMemoryHandle::SharedMemoryHandle()
    : fd(-1), address(nullptr), size(0), owner(false), unlink_on_destroy(false)
{
}

SharedMemoryHandle::SharedMemoryHandle(SharedMemoryHandle &&other) noexcept
    : fd(other.fd)
    , address(other.address)
    , size(other.size)
    , name(std::move(other.name))
    , owner(other.owner)
    , unlink_on_destroy(other.unlink_on_destroy)
{
    other.fd = -1;
    other.address = nullptr;
    other.size = 0;
    other.owner = false;
    other.unlink_on_destroy = false;
}

SharedMemoryHandle &SharedMemoryHandle::operator=(SharedMemoryHandle &&other) noexcept
{
    if (this == &other) {
        return *this;
    }
    this->~SharedMemoryHandle();
    fd = other.fd;
    address = other.address;
    size = other.size;
    name = std::move(other.name);
    owner = other.owner;
    unlink_on_destroy = other.unlink_on_destroy;

    other.fd = -1;
    other.address = nullptr;
    other.size = 0;
    other.owner = false;
    other.unlink_on_destroy = false;
    return *this;
}

SharedMemoryHandle::~SharedMemoryHandle()
{
    if (address != nullptr && size > 0) {
        munmap(address, size);
    }
    if (fd >= 0) {
        close(fd);
    }
    if (owner && unlink_on_destroy && !name.empty()) {
        shm_unlink(name.c_str());
    }
}

SharedMemoryHandle CreateSharedMemory(const std::string &name, std::size_t size, bool unlink_on_destroy)
{
    if (size == 0) {
        throw std::runtime_error("Shared memory size must be larger than 0");
    }

    SharedMemoryHandle handle;
    handle.name = EnsureSharedMemoryName(name);
    handle.owner = true;
    handle.unlink_on_destroy = unlink_on_destroy;
    handle.size = size;

    handle.fd = shm_open(handle.name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
    if (handle.fd < 0) {
        ThrowErrno("Failed to create shared memory");
    }

    if (ftruncate(handle.fd, static_cast<off_t>(size)) != 0) {
        ThrowErrno("Failed to resize shared memory");
    }

    handle.address = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, handle.fd, 0);
    if (handle.address == MAP_FAILED) {
        handle.address = nullptr;
        ThrowErrno("Failed to map shared memory");
    }

    return handle;
}

SharedMemoryHandle AttachSharedMemory(const std::string &name, bool read_only)
{
    SharedMemoryHandle handle;
    handle.name = EnsureSharedMemoryName(name);
    handle.owner = false;
    handle.unlink_on_destroy = false;

    const int open_flags = read_only ? O_RDONLY : O_RDWR;
    handle.fd = shm_open(handle.name.c_str(), open_flags, 0600);
    if (handle.fd < 0) {
        ThrowErrno("Failed to open shared memory");
    }

    struct stat sb {};
    if (fstat(handle.fd, &sb) != 0) {
        ThrowErrno("Failed to inspect shared memory");
    }
    if (sb.st_size <= 0) {
        throw std::runtime_error("Shared memory has invalid size");
    }
    handle.size = static_cast<std::size_t>(sb.st_size);

    const int prot = read_only ? PROT_READ : (PROT_READ | PROT_WRITE);
    handle.address = mmap(nullptr, handle.size, prot, MAP_SHARED, handle.fd, 0);
    if (handle.address == MAP_FAILED) {
        handle.address = nullptr;
        ThrowErrno("Failed to map shared memory");
    }

    return handle;
}

} // namespace HistogramInternal

#else

namespace HistogramInternal {

SharedMemoryHandle::SharedMemoryHandle()
    : fd(-1), address(nullptr), size(0), owner(false), unlink_on_destroy(false)
{
}

SharedMemoryHandle::SharedMemoryHandle(SharedMemoryHandle &&other) noexcept = default;
SharedMemoryHandle &SharedMemoryHandle::operator=(SharedMemoryHandle &&other) noexcept = default;
SharedMemoryHandle::~SharedMemoryHandle() = default;

SharedMemoryHandle CreateSharedMemory(const std::string &, std::size_t, bool)
{
    throw std::runtime_error("Shared memory is only supported on Linux and macOS");
}

SharedMemoryHandle AttachSharedMemory(const std::string &, bool)
{
    throw std::runtime_error("Shared memory is only supported on Linux and macOS");
}

} // namespace HistogramInternal

#endif
