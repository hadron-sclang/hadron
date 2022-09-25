#include "hadron/SourceFile.hpp"

#include "internal/FileSystem.hpp"

#include "spdlog/spdlog.h"

#include <fstream>

namespace hadron {

SourceFile::SourceFile(std::string path): m_path(path), m_codeSize(0) { }

bool SourceFile::read() {
    fs::path filePath(m_path);
    if (!fs::exists(filePath)) {
        SPDLOG_ERROR("File: '{}' not found", m_path);
        return false;
    }

    // Make room for the null terminator.
    m_codeSize = fs::file_size(filePath) + 1;
    m_code = std::make_unique<char[]>(m_codeSize);
    m_code[m_codeSize - 1] = '\0';
    std::ifstream inFile(filePath, std::ifstream::binary);
    if (!inFile) {
        SPDLOG_ERROR("File: '{}' open error", m_path);
        return false;
    }
    inFile.read(m_code.get(), m_codeSize - 1);
    if (!inFile) {
        SPDLOG_ERROR("File: '{}' read error", m_path);
        return false;
    }

    return true;
}

} // namespace hadron