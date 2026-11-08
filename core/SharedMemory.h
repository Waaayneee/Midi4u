#pragma once

#include <cstddef>
#include <memory>
#include <string>

namespace midi4u
{

// A named block of memory shared between processes. Windows uses a named file mapping,
// macOS uses POSIX shared memory. New memory starts zero filled.
class SharedMemory
{
public:
    // Opens the named block, creating it if needed. Returns nullptr and sets error on failure.
    static std::unique_ptr<SharedMemory> openOrCreate(const std::string& name, size_t bytes, std::string& error);

    // Removes the name so the next openOrCreate starts fresh. No-op on Windows, where the
    // block disappears with its last handle. Used by tests.
    static void removeName(const std::string& name);

    ~SharedMemory();

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    void* data() const { return data_; }
    size_t size() const { return size_; }

private:
    SharedMemory(void* data, size_t size, void* handle) : data_(data), size_(size), handle_(handle) {}

    void* data_;
    size_t size_;
    [[maybe_unused]] void* handle_; // Windows mapping handle, unused elsewhere
};

} // namespace midi4u
