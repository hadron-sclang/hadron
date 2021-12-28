// hlang, command line SuperCollider language script interpreter
#include "hadron/ErrorReporter.hpp"
#include "hadron/Runtime.hpp"

#include "gflags/gflags.h"

#include <iostream>
#include <memory>

DEFINE_string(sourceFile, "", "Path to the source code file to execute.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Runtime runtime(errorReporter);
    if (!runtime.initialize()) {
        return -1;
    }

/*
    hadron::Interpreter interpreter;
    if (!interpreter.setup()) {
        std::cerr << "Error setting up Hadron Interpreter";
        return -1;
    }

    auto function = interpreter.compileFile(FLAGS_sourceFile);
    if (!function) {
        std::cerr << "Fatal error compiling source file " << FLAGS_sourceFile << std::endl;
        return -1;
    }

    interpreter.run(function.get());
*/
    return 0;
}