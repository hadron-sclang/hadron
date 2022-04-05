// schema, parses a SuperCollider input class file and generates a Schema C++ output file
#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SourceFile.hpp"
#include "internal/FileSystem.hpp"

#include "fmt/format.h"
#include "gflags/gflags.h"

#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

DEFINE_string(classFiles, "", "Semicolon-delineated list of input class files to process.");
DEFINE_string(libraryPath, "", "Base path of the SC class library.");
DEFINE_string(schemaPath, "", "Base path of output schema files.");

namespace {
// While we generate a Schema struct for these objects they are not represented by Hadron with pointers, rather their
// values are packed into the Slot directly. So they are excluded from the Schema class heirarchy.
std::unordered_set<std::string> PrimitiveTypeNames {
    "Boolean",
    "Char",
    "Float",
    "Integer",
    "Nil",
    "Symbol"
};

// Some argument names in sclang are C++ keywords, causing the generated file to have invalid code. We keep a list
// here of suitable replacements and use them if a match is encountered.
std::unordered_map<std::string, std::string> keywordSubs {
    {"bool", "scBool"}
};

struct ClassInfo {
    std::string className;
    std::string superClassName;
    bool isPrimitiveType;
    std::vector<std::string> variables;
};
} // namespace

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();

    // Map of class names to info.
    std::unordered_map<std::string, ClassInfo> classes;
    // Map of paths to in-order class names to define.
    std::unordered_map<std::string, std::vector<std::string>> classFiles;

    // Parse semicolon-delinated list of input class files.
    size_t pathBegin = 0;
    size_t pathEnd = FLAGS_classFiles.find_first_of(';');
    do {
        fs::path classFile = pathEnd == std::string::npos ? FLAGS_classFiles.substr(pathBegin) :
                FLAGS_classFiles.substr(pathBegin, pathEnd - pathBegin);

        if (!fs::exists(classFile)) {
            std::cerr << "Class file path: " << classFile.string() << " does not exist.";
            return -1;
        }

        hadron::SourceFile sourceFile(classFile);
        if (!sourceFile.read(errorReporter)) {
            std::cerr << "Failed to read input class file: " << classFile << std::endl;
            return -1;
        }
        auto code = sourceFile.codeView();
        errorReporter->setCode(code.data());

        hadron::Lexer lexer(code, errorReporter);
        if (!lexer.lex() || !errorReporter->ok()) {
            std::cerr << "schema failed to lex input class file: " << classFile << std::endl;
            return -1;
        }

        hadron::Parser parser(&lexer, errorReporter);
        if (!parser.parseClass() || !errorReporter->ok()) {
            std::cerr << "schema failed to parse input class file: " << classFile << std::endl;
            return -1;
        }

        // Place an empty vector for appending class names.
        std::vector<std::string> classNames;

        const hadron::parse::Node* node = parser.root();
        while (node) {
            if (node->nodeType != hadron::parse::NodeType::kClass) {
                std::cerr << "No Class root node in parse tree for file: " << classFile << std::endl;
                return -1;
            }
            const hadron::parse::ClassNode* classNode = reinterpret_cast<const hadron::parse::ClassNode*>(node);

            ClassInfo classInfo;

            auto token = lexer.tokens()[classNode->tokenIndex];
            classInfo.className = std::string(token.range);
            classNames.emplace_back(classInfo.className);

            if (classInfo.className != "Object") {
                if (classNode->superClassNameIndex) {
                    token = lexer.tokens()[classNode->superClassNameIndex.value()];
                    classInfo.superClassName = std::string(token.range);
                } else {
                    classInfo.superClassName = "Object";
                }
            }

            classInfo.isPrimitiveType = PrimitiveTypeNames.count(classInfo.className) != 0;

            // Add instance variables to classInfo struct.
            const hadron::parse::VarListNode* varList = classNode->variables.get();
            while (varList) {
                if (lexer.tokens()[varList->tokenIndex].hash == hadron::kVarHash) {
                    const hadron::parse::VarDefNode* varDef = varList->definitions.get();
                    while (varDef) {
                        classInfo.variables.emplace_back(std::string(lexer.tokens()[varDef->tokenIndex].range));
                        varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
                    }
                }
                varList = reinterpret_cast<const hadron::parse::VarListNode*>(varList->next.get());
            }

            classes.emplace(std::make_pair(classInfo.className, std::move(classInfo)));

            node = classNode->next.get();
        }

        classFiles.emplace(std::make_pair(classFile, std::move(classNames)));

        if (pathEnd == std::string::npos) { break; }
        pathBegin = pathEnd + 1;
        pathEnd = FLAGS_classFiles.find_first_of(';', pathBegin);
    } while (true);



    return -1;
/*

    std::ofstream outFile(FLAGS_schemaFile);
    if (!outFile) {
        std::cerr << "schema file error on ouput file: " << FLAGS_schemaFile << std::endl;
        return -1;
    }

    std::ofstream caseFile(FLAGS_caseFile);
    if (!caseFile) {
        std::cerr << "schema file error on case file: " << FLAGS_caseFile << std::endl;
        return -1;
    }


    fs::path outFilePath(FLAGS_schemaFile);
    auto includeGuard = fmt::format("SRC_HADRON_SCHEMA_{:012X}", hadron::hash(FLAGS_schemaFile));
    outFile << "#ifndef " << includeGuard << std::endl;
    outFile << "#define " << includeGuard << std::endl << std::endl;

    outFile << "// NOTE: schemac generated this file from sclang input file:" << std::endl;
    outFile << "// " << FLAGS_classFile << std::endl;
    outFile << "// edits will likely be clobbered." << std::endl << std::endl;

    outFile << "namespace hadron {" << std::endl;
    outFile << "namespace schema {" << std::endl << std::endl;


        outFile << "// ========== " << className << std::endl;
        if (isPrimitiveType) {
            outFile << fmt::format("struct {}Schema {{\n", className);
        } else {
            if (className == "Object") {
                outFile << "struct ObjectSchema : public library::Schema {\n";
            } else {
                outFile << fmt::format("struct {}Schema : public {}Schema {{\n", className, superClassName);
            }
        }

        outFile << fmt::format("    static constexpr Hash kNameHash = 0x{:012x};\n", hadron::hash(className));
        outFile << fmt::format("    static constexpr Hash kMetaNameHash = 0x{:012x};\n\n",
                hadron::hash(fmt::format("Meta_{}", className)));

        // Add member variables to struct definition.
        const hadron::parse::VarListNode* varList = classNode->variables.get();
        while (varList) {
            if (lexer.tokens()[varList->tokenIndex].hash == hadron::kVarHash) {
                const hadron::parse::VarDefNode* varDef = varList->definitions.get();
                while (varDef) {
                    outFile << fmt::format("    Slot {};\n", lexer.tokens()[varDef->tokenIndex].range);
                    varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
                }
            }
            varList = reinterpret_cast<const hadron::parse::VarListNode*>(varList->next.get());
        }

        std::map<std::string, std::string> primitives;
        std::map<std::string, std::string> caseBlocks;

        // TODO: need to differentiate between Class methods (put them in Meta_ClassName structs) and instance methods.

        // Add any primitives as member functions.
        const hadron::parse::MethodNode* method = classNode->methods.get();
        while (method) {
            if (method->primitiveIndex) {
                std::string primitiveName(lexer.tokens()[method->primitiveIndex.value()].range);
                // Uniqueify the primitive calls, as they can occur in more than one method.
                if (primitives.find(primitiveName) == primitives.end()) {
                    std::string signature = fmt::format("    static Slot {}(ThreadContext* context, Slot _this",
                            primitiveName);
                    std::string caseBlock = fmt::format("// {}:{}\ncase 0x{:012x}: {{\n", className,
                            primitiveName, hadron::hash(primitiveName));
                    std::vector<std::string> argNames;
                    if (method->body && method->body->arguments && method->body->arguments->varList) {
                        // TODO: vargargs? Is there such a thing as a varargs primitive?
                        const hadron::parse::VarDefNode* varDef = method->body->arguments->varList->definitions.get();
                        while (varDef) {
                            std::string varName(lexer.tokens()[varDef->tokenIndex].range);
                            auto subs = keywordSubs.find(varName);
                            if (subs != keywordSubs.end()) { varName = subs->second; }
                            signature += fmt::format(", Slot {}", varName);
                            argNames.emplace_back(varName);
                            varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
                        }
                    }

                    signature += ");\n";
                    primitives.emplace(std::make_pair(primitiveName, signature));

                    size_t numberOfArgs = argNames.size() + 1;
                    caseBlock += fmt::format("    Slot _this = *(sp + {});\n", numberOfArgs);
                    for (size_t i = 0; i < argNames.size(); ++i) {
                        caseBlock += fmt::format("    Slot {} = *(sp + {});\n", argNames[i], numberOfArgs - i - 1);
                    }
                    caseBlock += fmt::format("    return library::{}::{}(context, _this", className, primitiveName);
                    for (const auto& arg : argNames) {
                        caseBlock += fmt::format(", {}", arg);
                    }
                    caseBlock += ");\n}\n\n";
                    caseBlocks.emplace(std::make_pair(primitiveName, caseBlock));
                }
            }
            method = reinterpret_cast<const hadron::parse::MethodNode*>(method->next.get());
        }

        // Output all primitives in sorted order.
        for (auto primitive : primitives) {
            outFile << primitive.second;
        }
        for (auto caseBlock : caseBlocks) {
            caseFile << caseBlock.second;
        }

        outFile << "};" << std::endl << std::endl;


    outFile << "} // namespace library" << std::endl;
    outFile << "} // namespace hadron" << std::endl << std::endl;

    outFile << "#endif // " << includeGuard << std::endl;
    return 0;
*/
}