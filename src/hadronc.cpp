// hadronc is a command-line sclang compiler.
#include "hadron/CodeGenerator.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Function.hpp"
#include "hadron/JITMemoryArena.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/MachineCodeRenderer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "hadron/ThreadContext.hpp"
#include "hadron/VirtualJIT.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>

DEFINE_string(inputFile, "", "path to input file to process");
DEFINE_bool(printGeneratedCode, false, "print the virtual machine assembler to the console");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::set_level(spdlog::level::debug);

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::SourceFile file(FLAGS_inputFile);
    if (!file.read(errorReporter)) {
        return -1;
    }

    hadron::Lexer lexer(file.codeView(), errorReporter);
    if (!lexer.lex() || !errorReporter->ok()) {
        return -1;
    }

    hadron::Parser parser(&lexer, errorReporter);
    if (!parser.parse() || !errorReporter->ok()) {
        return -1;
    }

    hadron::SyntaxAnalyzer analyzer(&parser, errorReporter);
    if (!analyzer.buildAST() || !errorReporter->ok()) {
        return -1;
    }

    //**  check that root AST is a block type.
    if (analyzer.ast()->astType != hadron::ast::ASTType::kBlock) {
        errorReporter->addError("Not a block!");
        return -1;
    }

    const hadron::ast::BlockAST* blockAST = reinterpret_cast<const hadron::ast::BlockAST*>(analyzer.ast());
    hadron::CodeGenerator generator(blockAST, errorReporter);
    if (!generator.generate() || !errorReporter->ok()) {
        return -1;
    }

    if (FLAGS_printGeneratedCode) {
        std::string codeString;
        if (!generator.virtualJIT()->toString(codeString)) {
            return -1;
        }
        std::cout << codeString << std::endl;
    }

    return 0;
}