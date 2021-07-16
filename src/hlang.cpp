// hlang is a command-line sclang interpreter.
#include "hadron/CompilerContext.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Slot.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_string(inputFile, "", "path to input file to process");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    hadron::CompilerContext::initJITGlobals();

    // Read the input file into a compiler context.
    hadron::CompilerContext cc(FLAGS_inputFile);
    if (!cc.readFile()) {
        return -1;
    }

    hadron::Slot result;
    if (!cc.evaluate(&result)) {
        std::cerr << "Error evaluating file input '" << FLAGS_inputFile << "." << std::endl;
        return -1;
    }

    std::cout << result.asString();

    hadron::CompilerContext::finishJITGlobals();

    return 0;
}