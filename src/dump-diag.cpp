// dump-diag, utility to print compilation diagnostics to stdout in JSON.
#include "hadron/ASTBuilder.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/HadronBuildArtifacts.hpp"
#include "hadron/library/HadronParseNode.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SlotDumpJSON.hpp"
#include "hadron/SlotDumpSCLang.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/ThreadContext.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_string(sourceFile, "", "Path to the source code file to process.");
DEFINE_bool(dumpClassArray, false, "Dump the compiled class library data structures, then exit.");
DEFINE_bool(dumpSCLang, false, "If true, dump sclang code. If false, dump JSON.");
DEFINE_int32(stopAfter, 7, "Stop compilation after phase, a number from 1-7. Compilation phases are: \n"
                           "    1: parse\n"
                           "    2: ast\n"
                           "    3: cfg\n"
                           "    4: linear frame\n"
                           "    5: lifetime analysis\n"
                           "    6: register allocation\n"
                           "    7: machine code\n");

// buildArtifacts should already have sourceFile, className, methodName, and parseTree specified. This function fills
// out the rest of the buildArtifacts object, up until stopAfter.
void build(hadron::ThreadContext* context, hadron::library::BuildArtifacts buildArtifacts, int stopAfter,
        std::shared_ptr<hadron::ErrorReporter> errorReporter) {
    if (stopAfter < 2) { return; }
    hadron::ASTBuilder astBuilder(errorReporter);
    buildArtifacts.setAbstractSyntaxTree(astBuilder.buildBlock(context,
            hadron::library::BlockNode(buildArtifacts.parseTree().slot())));

    if (stopAfter < 3) { return; }

    auto classDef = context->classLibrary->findClassNamed(buildArtifacts.className(context));
    assert(classDef);
    if (!classDef) { return; }
    hadron::BlockBuilder blockBuilder(errorReporter);
    buildArtifacts.setControlFlowGraph(blockBuilder.buildMethod(context,
            buildArtifacts.abstractSyntaxTree(), classDef));
    assert(buildArtifacts.controlFlowGraph());
}

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);
    spdlog::default_logger()->set_level(spdlog::level::warn);

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    hadron::Runtime runtime(errorReporter);
    if (!runtime.initInterpreter()) {
        return -1;
    }

    if (FLAGS_stopAfter > 2) {
        if (!runtime.compileClassLibrary()) {
            return -1;
        }
    }

    if (FLAGS_dumpClassArray) {
        auto dump = hadron::SlotDumpJSON();
        dump.dump(runtime.context(), runtime.context()->classLibrary->classArray().slot());
        std::cout << dump.json() << std::endl;
        return 0;
    }

    fs::path sourcePath(FLAGS_sourceFile);
    bool isClassFile = sourcePath.extension() == ".sc";

    auto sourceFile = std::make_unique<hadron::SourceFile>(sourcePath.string());
    if (!sourceFile->read(errorReporter)) { return -1; }

    auto lexer = std::make_unique<hadron::Lexer>(sourceFile->codeView(), errorReporter);
    bool lexResult = lexer->lex();
     if (!lexResult) { return -1; }

    auto parser = std::make_unique<hadron::Parser>(lexer.get(), errorReporter);
    bool parseResult;
    if (isClassFile) {
        parseResult = parser->parseClass(runtime.context());
    } else {
        parseResult = parser->parse(runtime.context());
    }

    if (!parseResult) { return -1; }

    auto artifacts = hadron::library::TypedArray<hadron::library::BuildArtifacts>::typedArrayAlloc(runtime.context());
    auto sourceFileSymbol = hadron::library::Symbol::fromView(runtime.context(), FLAGS_sourceFile);

    if (isClassFile) {
        auto rootNode = parser->root();
        while (rootNode) {
            auto className = rootNode.token().snippet(runtime.context());

            hadron::library::MethodNode methodNode;
            if (rootNode.className() == hadron::library::ClassNode::nameHash()) {
                auto classNode = hadron::library::ClassNode(rootNode.slot());
                methodNode = classNode.methods();
            } else {
                auto classExtNode = hadron::library::ClassExtNode(rootNode.slot());
                methodNode = classExtNode.methods();
            }

            while (methodNode) {
                auto buildArtifacts = hadron::library::BuildArtifacts::make(runtime.context());
                buildArtifacts.setSourceFile(sourceFileSymbol);
                buildArtifacts.setClassName(className);
                if (methodNode.isClassMethod()) {
                    // Prepend a '*' symbol to class method names.
                    buildArtifacts.setMethodName(hadron::library::Symbol::fromView(runtime.context(),
                            fmt::format("*{}", methodNode.token().snippet(runtime.context()).view(runtime.context()))));
                } else {
                    buildArtifacts.setMethodName(methodNode.token().snippet(runtime.context()));
                }
                buildArtifacts.setParseTree(methodNode.body().toBase());
                build(runtime.context(), buildArtifacts, FLAGS_stopAfter, errorReporter);
                artifacts = artifacts.typedAdd(runtime.context(), buildArtifacts);
                methodNode = hadron::library::MethodNode(methodNode.next().slot());
            }

            rootNode = rootNode.next();
        }
    } else {
        auto buildArtifacts = hadron::library::BuildArtifacts::make(runtime.context());
        buildArtifacts.setSourceFile(sourceFileSymbol);
        buildArtifacts.setParseTree(parser->root());
        build(runtime.context(), buildArtifacts, FLAGS_stopAfter, errorReporter);
        artifacts = artifacts.typedAdd(runtime.context(), buildArtifacts);
    }

    if (FLAGS_dumpSCLang) {
        hadron::SlotDumpSCLang::dump(runtime.context(), artifacts.slot(), std::cout);
    } else {
        auto dump = hadron::SlotDumpJSON();
        dump.dump(runtime.context(), artifacts.slot());
        std::cout << dump.json() << std::endl;
    }

    return 0;
}