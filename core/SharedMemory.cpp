#include "SharedMemory.h"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <cerrno>
    #include <cstring>
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

namespace midi4u
{

#if defined(_WIN32)

std::unique_ptr<SharedMemory> SharedMemory::openOrCreate(const std::string& name, size_t bytes, std::string& error)
{
    const auto size64 = static_cast<unsigned long long>(bytes);
    HANDLE mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                        static_cast<DWORD>(size64 >> 32), static_cast<DWORD>(size64 & 0xffffffffull),
                                        name.c_str());
    if (mapping == nullptr)
    {
        error = "CreateFileMapping failed, error " + std::to_string(GetLastError());
        return nullptr;
    }

    void* view = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, bytes);

    if (view == nullptr)
    {
        error = "MapViewOfFile failed, error " + std::to_string(GetLastError());
        CloseHandle(mapping);
        return nullptr;
    }

    return std::unique_ptr<SharedMemory>(new SharedMemory(view, bytes, mapping));
}

void SharedMemory::removeName(const std::string&) {}

SharedMemory::~SharedMemory()
{
    UnmapViewOfFile(data_);
    CloseHandle(static_cast<HANDLE>(handle_));
}

#else

std::unique_ptr<SharedMemory> SharedMemory::openOrCreate(const std::string& name, size_t bytes, std::string& error)
{
    const int fd = shm_open(name.c_str(), O_RDWR | O_CREAT, 0600);

    if (fd < 0)
    {
        error = std::string("shm_open failed: ") + std::strerror(errno);
        return nullptr;
    }

    struct stat info {};

    if (fstat(fd, &info) == 0 && info.st_size < static_cast<off_t>(bytes))
    {
        // macOS only lets the size be set once. If both sides race here, one call fails
        // and the size check below decides.
        (void) ftruncate(fd, static_cast<off_t>(bytes));
    }

    if (fstat(fd, &info) != 0 || info.st_size < static_cast<off_t>(bytes))
    {
        error = "Shared memory has the wrong size";
        close(fd);
        return nullptr;
    }

    void* view = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (view == MAP_FAILED)
    {
        error = std::string("mmap failed: ") + std::strerror(errno);
        return nullptr;
    }

    return std::unique_ptr<SharedMemory>(new SharedMemory(view, bytes, nullptr));
}

void SharedMemory::removeName(const std::string& name)
{
    shm_unlink(name.c_str());
}

SharedMemory::~SharedMemory()
{
    munmap(data_, size_);
}

#endif

} // namespace midi4u
