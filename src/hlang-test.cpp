// hlang, command line SuperCollider language script interpreter
#include "hadron/ErrorReporter.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SourceFile.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>
#include <string_view>
#include <string>

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    spdlog::default_logger()->set_level(spdlog::level::trace);

    if (argc != 2) {
        std::cerr << "usage: hlang-test [options] input-file.sctest" << std::endl;
        return -1;
    }

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Runtime runtime(errorReporter);
    if (!runtime.initInterpreter()) { return -1; }

    fs::path sourcePath(argv[1]);
    auto sourceFile = hadron::SourceFile(sourcePath.string());
    if (!sourceFile.read(errorReporter)) { return -1; }

    // Big difference between clang testing and hit is the input files are not necessarily valid as standalone sclang
    // files. What we need are three different sections - documentation sections/comments, class defs and class exts,
    // and interactive scripts for running and testing output from.
    // Class Defs could also cause errors.

    // Comments are just comments. Block comments are ignored.
    // CLASSES: <name>

    // RUN: <name>
    // <code>

    // In run or class fields, any of these terminate the input:
    // EXPECTING: <results string>
    // ERROR: <could put number here?>
    // WARN: <could put number here?>

    // Parse file extracting only line comments with supported tags.
    // Then there's a state machine


    return 0;
}