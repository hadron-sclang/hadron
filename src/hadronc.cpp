// hadronc is a command-line sclang compiler.
#include "FileSystem.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/LightningJIT.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SyntaxAnalyzer.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <memory>

DEFINE_string(inputFile, "", "path to input file to process");
DEFINE_bool(printJIT, true, "print the GNU lightning bytecode to the console");

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
        spdlog::error("Failed to open input file {}", filePath.string());
        return -1;
    }
    inFile.read(fileContents.get(), fileSize);
    if (!inFile) {
        spdlog::error("Failed to read file {}", filePath.string());
        return -1;
    }

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Parser parser(std::string_view(fileContents.get(), fileSize), errorReporter);
    if (!parser.parse()) {
        spdlog::error("Failed to parse file {}", filePath.string());
        return -1;
    }

    hadron::SyntaxAnalyzer syntaxAnalyzer(errorReporter);
    if (!syntaxAnalyzer.buildAST(&parser)) {
        spdlog::error("Syntax analysis for file {} failed.", filePath.string());
        return -1;
    }

    hadron::LightningJIT::initJITGlobals();
    hadron::LightningJIT jit;
    auto block = jit.jitBlock(reinterpret_cast<const hadron::ast::BlockAST*>(syntaxAnalyzer.ast()));
    if (!block) {
        spdlog::error("Failed to jit block for file {}.", filePath.string());
    }
    block->printJIT();
    block.reset();
    hadron::LightningJIT::finishJITGlobals();

    return 0;
}