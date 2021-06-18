#include "FileSystem.hpp"

#include "ErrorReporter.hpp"
#include "Parser.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <fstream>
#include <memory>
#include <string_view>

DEFINE_string(inputFile, "", "path to input file to process");
DEFINE_bool(parseTree, false, "print the parse tree");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    fs::path filePath(FLAGS_inputFile);
    if (!fs::exists(filePath)) {
        spdlog::error("File '{}' does not exist.", filePath.string());
        return -1;
    }

    auto fileSize = fs::file_size(filePath);
    auto fileContents = std::make_unique<char[]>(fileSize);
    std::ifstream inFile(filePath);
    if (!inFile) {
        spdlog::error("Failed to open file {}", filePath.string());
        return -1;
    }
    inFile.read(fileContents.get(), fileSize);
    if (!inFile) {
        spdlog::error("Failed to read file {}", filePath.string());
        return -1;
    }

    if (FLAGS_parseTree) {
        auto errorReporter = std::make_shared<hadron::ErrorReporter>();
        hadron::Parser parser(std::string_view(fileContents.get(), fileSize), errorReporter);
        if (!parser.parse()) {
            spdlog::error("Failed to parse file {}", filePath.string());
            return -1;
        }
    }

    return 0;
}
