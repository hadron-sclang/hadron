// dump-diag, utility to print compilation diagnostics to stdout in JSON.
#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/HadronParseNode.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SlotDumpJSON.hpp"
#include "hadron/SourceFile.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_string(sourceFile, "", "Path to the source code file to process.");
DEFINE_bool(dumpClassArray, false, "Dump the compiled class library data structures.");
DEFINE_bool(dumpParseTree, false, "Dump the parse tree for the input file.");
DEFINE_bool(doesItParse, false, "Print YES if the file parsed successfully");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    if (FLAGS_doesItParse) {
        spdlog::default_logger()->set_level(spdlog::level::critical);
    } else {
        spdlog::default_logger()->set_level(spdlog::level::warn);
    }

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Runtime runtime(errorReporter);
    if (!runtime.initInterpreter()) {
        return -1;
    }

    if (FLAGS_dumpClassArray) {
        auto dump = hadron::SlotDumpJSON();
        dump.dump(runtime.context(), runtime.context()->classLibrary->classArray().slot());
        std::cout << dump.json() << std::endl;
    }

    fs::path sourcePath(FLAGS_sourceFile);
    bool isClassFile = sourcePath.extension() == ".sc";

    auto sourceFile = std::make_unique<hadron::SourceFile>(sourcePath.string());
    if (!sourceFile->read(errorReporter)) { return -1; }

    auto lexer = std::make_unique<hadron::Lexer>(sourceFile->codeView(), errorReporter);
    bool lexResult = lexer->lex();
    if (FLAGS_doesItParse) {
        if (!lexResult) {
            std::cout << "NO, IT DOESN'T EVEN LEX: " << sourcePath << std::endl;
            return 0;
        }
    } else if (!lexResult) { return -1; }

    auto parser = std::make_unique<hadron::Parser>(lexer.get(), errorReporter);
    bool parseResult;
    if (isClassFile) {
        parseResult = parser->parseClass(runtime.context());
    } else {
        parseResult = parser->parse(runtime.context());
    }

    if (FLAGS_doesItParse) {
        if (parseResult) {
            std::cout << "YES" << std::endl;
        }
        else {
            std::cout << "NO: " << sourcePath << std::endl;
        }
        return 0;
    } else {
        if (!parseResult) { return -1; }
    }

    if (FLAGS_dumpParseTree) {
        auto dump = hadron::SlotDumpJSON();
        dump.dump(runtime.context(), parser->root().slot());
        std::cout << dump.json() << std::endl;
    }

    return 0;
}