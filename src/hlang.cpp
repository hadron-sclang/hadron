// hlang is a command-line sclang interpreter.
#include "hadron/ErrorReporter.hpp"
#include "hadron/Function.hpp"
#include "hadron/Interpreter.hpp"
#include "hadron/Slot.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_string(inputFile, "", "path to input file to process");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    hadron::Interpreter interpreter;
    if (!interpreter.setup()) {
        SPDLOG_ERROR("Interpreter setup failed.");
        return -1;
    }

    auto function = interpreter.compileFile(FLAGS_inputFile);
    if (!function) {
        return -1;
    }

    hadron::Slot result = interpreter.run(function.get());
    std::cout << result.asString() << std::endl;

    return 0;
}