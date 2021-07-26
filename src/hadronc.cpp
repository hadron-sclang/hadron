// hadronc is a command-line sclang compiler.
#include "hadron/Compiler.hpp"
#include "hadron/ErrorReporter.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_string(inputFile, "", "path to input file to process");
DEFINE_bool(printGeneratedCode, false, "print the virtual machine assembler to the console");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    hadron::CompilerContext::initJITGlobals();

    // Read the input file into a compiler context.
    hadron::CompilerContext cc(FLAGS_inputFile);
    if (!cc.readFile()) {
        return -1;
    }

    if (FLAGS_printGeneratedCode) {
        if (!cc.generateCode()) {
            return -1;
        }
        std::string codeString;
        if (!cc.getGeneratedCodeAsString(codeString)) {
            return -1;
        }
        std::cout << codeString << std::endl;
    }

    hadron::CompilerContext::finishJITGlobals();

    return 0;
}