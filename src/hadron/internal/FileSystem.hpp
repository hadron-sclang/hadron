#ifndef SRC_HADRON_INTERNAL_FILE_SYSTEM_HPP_
#define SRC_HADRON_INTERNAL_FILE_SYSTEM_HPP_

#if (__APPLE__)
#    include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#else
#    if __has_include(<filesystem>)
#        include <filesystem>
namespace fs = std::filesystem;
#    else
#        include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#    endif
#endif // __APPLE__

namespace hadron {

// Return an absolute path to the running binary. OS-specific code.
fs::path findBinaryPath();
fs::path findSCClassLibrary();
fs::path findHLangClassLibrary();

} // namespace hadron

#endif // SRC_HADRON_INTERNAL_FILE_SYSTEM_HPP_
