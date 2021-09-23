// schema, parses a SuperCollider input class file and generates a Schema C++ output file
#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SourceFile.hpp"

#include "gflags/gflags.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

DEFINE_string(classFile, "", "Path to the SC class file to generate schema file from.");
DEFINE_string(schemaFile, "", "Path to save the schema output header file to.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::SourceFile sourceFile(FLAGS_classFile);
    if (!sourceFile.read(errorReporter)) {
        std::cerr << "schema parser failed to read input class file: " << FLAGS_classFile << std::endl;
        return -1;
    }
    auto code = sourceFile.codeView();
    errorReporter->setCode(code.data());

    hadron::Lexer lexer(code, errorReporter);
    if (!lexer.lex() || !errorReporter->ok()) {
        std::cerr << "schema failed to lex input class file: " << FLAGS_classFile << std::endl;
        return -1;
    }

    hadron::Parser parser(&lexer, errorReporter);
    if (!parser.parseClass() || !errorReporter->ok()) {
        std::cerr << "schema failed to parse input class file: " << FLAGS_classFile << std::endl;
        return -1;
    }

    std::ofstream outFile(FLAGS_schemaFile);
    if (!outFile) {
        std::cerr << "schema file error on ouput file: " << FLAGS_schemaFile << std::endl;
    }

    const hadron::parse::Node* node = parser.root();
    while (node) {
        if (node->nodeType != hadron::parse::NodeType::kClass) {
            std::cerr << "schema did not find a Class root node in parse tree for file: " << FLAGS_classFile
                    << std::endl;
        }
        const hadron::parse::ClassNode* classNode = reinterpret_cast<const hadron::parse::ClassNode*>(node);
        auto token = lexer.tokens()[classNode->tokenIndex];
        std::string className(token.range.data(), token.range.size());
        std::cout << className << std::endl;

        node = classNode->next.get();
    }

    return 0;
}