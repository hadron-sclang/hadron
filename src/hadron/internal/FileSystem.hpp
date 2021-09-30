#ifndef SRC_FILE_SYSTEM_HPP_
#define SRC_FILE_SYSTEM_HPP_

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

#endif // SRC_FILE_SYSTEM_HPP_
