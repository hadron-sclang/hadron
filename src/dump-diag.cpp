// dump-diag, utility to print compilation diagnostics to stdout in JSON.
#include "hadron/ASTBuilder.hpp"
#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/ClassLibrary.hpp"
#include "hadron/internal/FileSystem.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/HadronBuildArtifacts.hpp"
#include "hadron/library/HadronParseNode.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Runtime.hpp"
#include "hadron/SlotDumpJSON.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include "gflags/gflags.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>

DEFINE_bool(pretty, false, "Pretty-print the dumped JSON.");
DEFINE_bool(dumpClassArray, false, "Dump the compiled class library data structures, then exit.");
DEFINE_bool(debug, false, "Debug mode");
DEFINE_int32(stopAfter, 7,
             "Stop compilation after phase, a number from 1-7. Compilation phases are: \n"
             "    1: parse\n"
             "    2: ast\n"
             "    3: cfg\n"
             "    4: linear frame\n"
             "    5: lifetime analysis\n"
             "    6: register allocation\n"
             "    7: machine code\n");

// buildArtifacts should already have sourceFile, className, methodName, and parseTree specified. This function fills
// out the rest of the buildArtifacts object, up until stopAfter.
void build(hadron::ThreadContext* context, hadron::library::BuildArtifacts buildArtifacts, int stopAfter) {
    if (stopAfter < 2) {
        return;
    }
    hadron::ASTBuilder astBuilder;
    buildArtifacts.setAbstractSyntaxTree(
        astBuilder.buildBlock(context, hadron::library::BlockNode(buildArtifacts.parseTree().slot())));

    if (stopAfter < 3) {
        return;
    }

    auto classDef = context->classLibrary->findClassNamed(buildArtifacts.className(context));
    assert(classDef);
    if (!classDef) {
        return;
    }
    auto method = hadron::library::Method();
    for (int32_t i = 0; i < classDef.methods().size(); ++i) {
        if (classDef.methods().typedAt(i).name(context) == buildArtifacts.methodName(context)) {
            method = classDef.methods().typedAt(i);
            break;
        }
    }
    if (!method) {
        return;
    }
    hadron::BlockBuilder blockBuilder(method);
    auto cfgFrame =
        blockBuilder.buildMethod(context, buildArtifacts.abstractSyntaxTree(), !buildArtifacts.methodName(context));
    if (!cfgFrame) {
        return;
    }
    buildArtifacts.setControlFlowGraph(cfgFrame);

    if (stopAfter < 4) {
        return;
    }

    // TODO: The reason for Materializer is that it needs to compile inner blocks first. So this dump-diag probably
    // fails somehow when trying to compile stuff and stopping halfway.
}

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    spdlog::default_logger()->set_level(spdlog::level::warn);

    hadron::Runtime runtime(FLAGS_debug);
    if (!runtime.initInterpreter()) {
        return -1;
    }

    if (FLAGS_dumpClassArray) {
        auto dump = hadron::SlotDumpJSON();
        dump.dump(runtime.context(), runtime.context()->classLibrary->classArray().slot(), FLAGS_pretty);
        std::cout << dump.json() << std::endl;
        return 0;
    }

    if (argc != 2) {
        std::cerr << "usage: dump-diag [flags] source_file\n";
        return -1;
    }

    std::string flagSourceFile(argv[1]);
    fs::path sourcePath(flagSourceFile);
    bool isClassFile = sourcePath.extension() == ".sc";

    auto sourceFile = std::make_unique<hadron::SourceFile>(sourcePath.string());
    if (!sourceFile->read()) {
        return -1;
    }

    auto lexer = std::make_unique<hadron::Lexer>(sourceFile->codeView());
    bool lexResult = lexer->lex();
    if (!lexResult) {
        return -1;
    }

    auto parser = std::make_unique<hadron::Parser>(lexer.get());
    bool parseResult;
    if (isClassFile) {
        parseResult = parser->parseClass(runtime.context());
    } else {
        parseResult = parser->parse(runtime.context());
    }

    if (!parseResult) {
        return -1;
    }

    auto artifacts = hadron::library::TypedArray<hadron::library::BuildArtifacts>::typedArrayAlloc(runtime.context());
    auto sourceFileSymbol = hadron::library::Symbol::fromView(runtime.context(), flagSourceFile);

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
                    buildArtifacts.setMethodName(hadron::library::Symbol::fromView(
                        runtime.context(),
                        fmt::format("*{}", methodNode.token().snippet(runtime.context()).view(runtime.context()))));
                } else {
                    buildArtifacts.setMethodName(methodNode.token().snippet(runtime.context()));
                }
                buildArtifacts.setParseTree(methodNode.body().toBase());
                build(runtime.context(), buildArtifacts, FLAGS_stopAfter);
                artifacts = artifacts.typedAdd(runtime.context(), buildArtifacts);
                methodNode = hadron::library::MethodNode(methodNode.next().slot());
            }

            rootNode = rootNode.next();
        }
    } else {
        auto buildArtifacts = hadron::library::BuildArtifacts::make(runtime.context());
        buildArtifacts.setSourceFile(sourceFileSymbol);
        buildArtifacts.setParseTree(parser->root());
        buildArtifacts.setClassName(runtime.context()->symbolTable->interpreterSymbol());
        buildArtifacts.setMethodName(runtime.context()->symbolTable->functionCompileContextSymbol());
        build(runtime.context(), buildArtifacts, FLAGS_stopAfter);
        artifacts = artifacts.typedAdd(runtime.context(), buildArtifacts);
    }

    auto dump = hadron::SlotDumpJSON();
    dump.dump(runtime.context(), artifacts.slot(), FLAGS_pretty);
    std::cout << dump.json() << std::endl;

    return 0;
}