#include "ClassLibrary.hpp"

#include "ErrorReporter.hpp"
#include "Parser.hpp"

#include "spdlog/spdlog.h"

#include <fstream>
#include <memory>
#include <string_view>

namespace hadron {

bool ClassLibrary::parseFile(const fs::path& filePath) {
    auto fileSize = fs::file_size(filePath);
    auto fileContents = std::make_unique<char[]>(fileSize);
    std::ifstream inFile(filePath);
    if (!inFile) {
        spdlog::error("Class Library failed to open file {}", filePath.string());
        return false;
    }
    inFile.read(fileContents.get(), fileSize);
    if (!inFile) {
        spdlog::error("Class Library failed to read file {}", filePath.string());
        return false;
    }

    auto errorReporter = std::make_shared<ErrorReporter>();
    Parser parser(std::string_view(fileContents.get(), fileSize), errorReporter);
    if (!parser.parse()) {
        spdlog::error("Class Library failed to parse file {}", filePath.string());
        return false;
    }

    return true;
}

} // namespace hadron
