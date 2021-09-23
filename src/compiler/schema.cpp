// schema, parses a SuperCollider input class file and generates a Schema C++ output file
#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/SourceFile.hpp"

#include "gflags/gflags.h"

#include <iostream>
#include <memory>

DEFINE_string(classFile, "", "Path to the SC class file to generate schema file from.");
DEFINE_string(schemaFile, "", "Path to save the schema output file to.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::SourceFile sourceFile(FLAGS_classFile);
    if (!sourceFile.read(errorReporter)) {
        std::cerr << "schema parser failed to read input class file: " << FLAGS_classFile << std::endl;
        return -1;
    }
//    Lexer lexer()

    return 0;
}