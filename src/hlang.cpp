// hlang, command line SuperCollider language script interpreter
#include "hadron/Runtime.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_string(sourceFile, "", "Path to the source code file to execute.");
DEFINE_bool(debug, false, "Run code in debug mode.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    spdlog::default_logger()->set_level(spdlog::level::trace);

    hadron::Runtime runtime(FLAGS_debug);
    if (!runtime.initInterpreter()) {
        return -1;
    }
    runtime.addDefaultPaths();
    if (!runtime.scanClassFiles()) {
        return -1;
    }
    if (!runtime.finalizeClassLibrary()) {
        return -1;
    }

    return 0;
}