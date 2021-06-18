#ifndef SRC_CLASS_LIBRARY_HPP_
#define SRC_CLASS_LIBRARY_HPP_

#include "FileSystem.hpp"

namespace hadron {

// Maintains the authoritative compiled binary code representing the sclang class library.
class ClassLibrary {
public:
    bool parseFile(const fs::path& filePath);
};

} // namespace hadron

#endif // SRC_CLASS_LIBRARY_HPP_
