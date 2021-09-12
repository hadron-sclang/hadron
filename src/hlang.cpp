// hlang, command line SuperCollider language script interpreter
#include "hadron/Function.hpp"
#include "hadron/Interpreter.hpp"

#include "gflags/gflags.h"

#include <iostream>

DEFINE_string(sourceFile, "", "Path to the source code file to execute.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
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
    return 0;
}