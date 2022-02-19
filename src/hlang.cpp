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
    if (!runtime.initInterpreter()) {
        return -1;
    }

    return 0;
}