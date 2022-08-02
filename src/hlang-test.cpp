// hlang, command line SuperCollider language script interpreter
#include "hadron/ErrorReporter.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SourceFile.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    spdlog::default_logger()->set_level(spdlog::level::trace);

    if (argc != 2) {
        std::cerr << "usage: hlang-test [options] input_file.sctest" << std::endl;
        return -1;
    }

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Runtime runtime(errorReporter);
    if (!runtime.initInterpreter()) { return -1; }

    fs::path sourcePath(argv[1]);
    auto sourceFile = hadron::SourceFile(sourcePath.string());
    if (!sourceFile.read(errorReporter)) { return -1; }

    return 0;
}