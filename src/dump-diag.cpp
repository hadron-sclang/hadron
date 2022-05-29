// dump-diag, utility to print compilation diagnostics to stdout in JSON.
#include "hadron/ErrorReporter.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/HadronParseNode.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SourceFile.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_string(sourceFile, "", "Path to the source code file to process.");
DEFINE_bool(dumpClassArray, true, "Dump the compiled class library data structures.")
DEFINE_bool(dumpParseTree, false, "Dump the parse tree for the input file.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    spdlog::default_logger()->set_level(spdlog::level::warn);

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Runtime runtime(errorReporter);
    if (!runtime.initInterpreter()) {
        return -1;
    }

    if (FLAGS_dumpClassArray) {
        

    }

    fs::path sourcePath(FLAGS_sourceFile);
    bool isClassFile = sourcePath.extension() == ".sc";

    auto sourceFile = std::make_unique<hadron::SourceFile>(sourcePath.string());
    if (!sourceFile->read(errorReporter)) { return -1; }

    auto lexer = std::make_unique<hadron::Lexer>(sourceFile->codeView(), errorReporter);
    if(!lexer->lex()) { return -1; }

    auto parser = std::make_unique<hadron::Parser>(lexer.get(), errorReporter);
    if (isClassFile) {
        if (!parser->parseClass(runtime.context())) { return -1; }
    } else {
        if (!parser->parse(runtime.context())) { return -1; }
    }

    if (FLAGS_dumpParseTree) {

    }

    return 0;
}