#include "hadron/SourceFile.hpp"

#include "hadron/ErrorReporter.hpp"
#include "internal/FileSystem.hpp"

#include <fstream>

namespace hadron {

SourceFile::SourceFile(std::string path): m_path(path), m_codeSize(0) { }

bool SourceFile::read(std::shared_ptr<ErrorReporter> errorReporter) {
    fs::path filePath(m_path);
    if (!fs::exists(filePath)) {
        errorReporter->addFileNotFoundError(m_path);
        return false;
    }

    m_codeSize = fs::file_size(filePath);
    m_code = std::make_unique<char[]>(m_codeSize);
    std::ifstream inFile(filePath);
    if (!inFile) {
        errorReporter->addFileOpenError(m_path);
        return false;
    }
    inFile.read(m_code.get(), m_codeSize);
    if (!inFile) {
        errorReporter->addFileReadError(m_path);
        return false;
    }

    return true;
}

} // namespace hadron