#include "FileSystem.hpp"

#include "spdlog/spdlog.h"

#if (__APPLE__)
#    include <mach-o/dyld.h>
#    include <array>
#endif // __APPLE__

#if WIN32
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#    include <array>
#endif

#include <cassert>

namespace hadron {

#if (__APPLE__)
fs::path findBinaryPath() {
    std::array<char, 4096> pathBuffer;
    uint32_t bufferSize = pathBuffer.size();
    auto ret = _NSGetExecutablePath(pathBuffer.data(), &bufferSize);
    if (ret >= 0) {
        return fs::canonical(fs::path(pathBuffer.data()).parent_path());
    }
    SPDLOG_ERROR("Failed to find path of executable!");
    return fs::path();
}
#elif defined(__linux__)
fs::path findBinaryPath() {
    auto path = fs::read_symlink("/proc/self/exe");
    return fs::canonical(path);
}
#elif WIN32
fs::path findBinaryPath() {
    std::array<char, 4096> pathBuffer;
    auto size = GetModuleFileNameA(nullptr, pathBuffer.data(), static_cast<DWORD>(pathBuffer.size()));
    if (size == 0) {
        SPDLOG_ERROR("Failed to find path of executable!");
        return fs::path();
    }
    return fs::canonical(fs::path(pathBuffer.data()));
}
#else
#    error Need to define findBinaryPath() for this operating system.
#endif

fs::path findSCClassLibrary() {
    auto path = findBinaryPath();
    SPDLOG_INFO("Found binary path at {}", path.string());
    path += "/../../third_party/bootstrap/SCClassLibrary";
    path = fs::canonical(path);
    SPDLOG_INFO("Found Class Library path at {}", path.string());
    assert(fs::exists(path));
    assert(fs::is_directory(path));
    return path;
}

fs::path findHLangClassLibrary() {
    auto path = findBinaryPath();
    path += "/../../classes/HLang";
    path = fs::canonical(path);
    assert(fs::exists(path));
    assert(fs::is_directory(path));
    return path;
}

} // namespace hadron