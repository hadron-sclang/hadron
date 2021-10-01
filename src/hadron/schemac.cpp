// schema, parses a SuperCollider input class file and generates a Schema C++ output file
#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SourceFile.hpp"
#include "internal/FileSystem.hpp"

#include "fmt/format.h"
#include "gflags/gflags.h"

#include <fstream>
#include <iostream>
#include <map>
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
        return 0;
    }

    std::ofstream outFile(FLAGS_schemaFile);
    if (!outFile) {
        std::cerr << "schema file error on ouput file: " << FLAGS_schemaFile << std::endl;
    }

    // Some argument names in sclang are C++ keywords, causing the generated file to have invalid code. We keep a list
    // here of suitable replacements and use them if a match is encountered.
    std::map<std::string, std::string> keywordSubs{{"bool", "scBool"}};

    fs::path outFilePath(FLAGS_schemaFile);
    auto includeGuard = fmt::format("SRC_HADRON_SCHEMA_{:012X}", hadron::hash(FLAGS_schemaFile).getHash());
    outFile << "#ifndef " << includeGuard << std::endl;
    outFile << "#define " << includeGuard << std::endl << std::endl;

    outFile << "#include \"hadron/ObjectHeader.hpp\"" << std::endl;
    outFile << "#include \"hadron/Slot.hpp\"" << std::endl;
    outFile << "#include \"hadron/ThreadContext.hpp\"" << std::endl;

    outFile << std::endl << "// NOTE: schemac generated this file from sclang input file:" << std::endl;
    outFile << "// " << FLAGS_classFile << std::endl;
    outFile << "// edits will likely be clobbered." << std::endl;

    outFile << std::endl << "namespace hadron {" << std::endl << std::endl;

    const hadron::parse::Node* node = parser.root();
    while (node) {
        if (node->nodeType != hadron::parse::NodeType::kClass) {
            std::cerr << "schema did not find a Class root node in parse tree for file: " << FLAGS_classFile
                    << std::endl;
        }
        const hadron::parse::ClassNode* classNode = reinterpret_cast<const hadron::parse::ClassNode*>(node);
        auto token = lexer.tokens()[classNode->tokenIndex];
        std::string className(token.range.data(), token.range.size());
        std::string superClassName;
        if (className != "Object") {
            if (classNode->superClassNameIndex) {
                token = lexer.tokens()[classNode->superClassNameIndex.value()];
                superClassName = std::string(token.range);
            } else {
                superClassName = "Object";
            }
        } else {
            superClassName = "ObjectHeader";
        }

        outFile << "// ========== " << className << std::endl;
        outFile << fmt::format("static constexpr uint64_t k{}Hash = 0x{:012x};\n\n", className,
                hadron::hash(className).getHash());

        outFile << fmt::format("struct {} : public {} {{\n", className, superClassName);

        // Add member variables to struct definition.
        const hadron::parse::VarListNode* varList = classNode->variables.get();
        while (varList) {
            if (lexer.tokens()[varList->tokenIndex].hash == hadron::hash("var")) {
                const hadron::parse::VarDefNode* varDef = varList->definitions.get();
                while (varDef) {
                    outFile << fmt::format("    Slot {};\n", lexer.tokens()[varDef->tokenIndex].range);
                    varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
                }
            }
            varList = reinterpret_cast<const hadron::parse::VarListNode*>(varList->next.get());
        }

        std::map<std::string, std::string> primitives;

        // Add any primitives as member functions.
        const hadron::parse::MethodNode* method = classNode->methods.get();
        while (method) {
            if (method->primitiveIndex) {
                std::string primitiveName(lexer.tokens()[method->primitiveIndex.value()].range);
                std::cerr << fmt::format("{}.{} has primitive: {}\n", className,
                        lexer.tokens()[method->tokenIndex].range, primitiveName);
                // Uniqueify the primitive calls, as they can occur in more than one method.
                if (primitives.find(primitiveName) == primitives.end()) {
                    std::string signature = fmt::format("    Slot {}(ThreadContext* context", primitiveName);
                    if (method->body && method->body->arguments && method->body->arguments->varList) {
                        // TODO: vargargs
                        // TODO: defaults
                        const hadron::parse::VarDefNode* varDef = method->body->arguments->varList->definitions.get();
                        while (varDef) {
                            std::string varName(lexer.tokens()[varDef->tokenIndex].range);
                            auto subs = keywordSubs.find(varName);
                            if (subs != keywordSubs.end()) { varName = subs->second; }
                            signature += fmt::format(", Slot {}", varName);
                            varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
                        }
                    }
                    signature += ");\n";
                    primitives.emplace(std::make_pair(primitiveName, signature));
                }
            } else {
                std::cerr << fmt::format("{}.{} no primitive\n", className,
                        lexer.tokens()[method->tokenIndex].range);
            }
            method = reinterpret_cast<const hadron::parse::MethodNode*>(method->next.get());
        }

        // Output all primitives in sorted order.
        for (auto primitive : primitives) {
            outFile << primitive.second;
        }

        outFile << fmt::format("}};\n\n");
        node = classNode->next.get();
    }

    outFile << "} // namespace hadron" << std::endl << std::endl;
    outFile << "#endif // " << includeGuard << std::endl;
    return 0;
}