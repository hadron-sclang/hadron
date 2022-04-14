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
    "RawPointer",
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

    fs::path libraryPath(FLAGS_libraryPath);
    if (!fs::exists(libraryPath)) {
        std::cerr << "Class library path does not exist: " << libraryPath << std::endl;
        return -1;
    }
    libraryPath = fs::absolute(libraryPath);

    fs::path schemaBasePath(FLAGS_schemaPath);
    schemaBasePath = fs::absolute(schemaBasePath);

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
        classFile = fs::absolute(classFile);

        if (!fs::exists(classFile)) {
            std::cerr << "Class file path: " << classFile.string() << " does not exist.";
            return -1;
        }

        // The class file must be in a subdirectory of the library path.
        if (classFile.string().substr(0, libraryPath.string().length()) != libraryPath.string()) {
            std::cerr << "Class file path: " << classFile.string() << " not in a subdirectory of library path: "
                    << libraryPath.string() << std::endl;
            return -1;
        }

        fs::path schemaPath = schemaBasePath /
            fs::path(classFile.string().substr(libraryPath.string().length())).parent_path().relative_path() /
            fs::path(classFile.stem().string() + "Schema.hpp");

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
                        std::string varName(lexer.tokens()[varDef->tokenIndex].range);
                        auto subs = keywordSubs.find(varName);
                        if (subs != keywordSubs.end()) { varName = subs->second; }
                        classInfo.variables.emplace_back(varName);
                        varDef = reinterpret_cast<const hadron::parse::VarDefNode*>(varDef->next.get());
                    }
                }
                varList = reinterpret_cast<const hadron::parse::VarListNode*>(varList->next.get());
            }

            classes.emplace(std::make_pair(classInfo.className, std::move(classInfo)));

            node = classNode->next.get();
        }

        classFiles.emplace(std::make_pair(schemaPath, std::move(classNames)));

        if (pathEnd == std::string::npos) { break; }
        pathBegin = pathEnd + 1;
        pathEnd = FLAGS_classFiles.find_first_of(';', pathBegin);
    } while (true);

    // Now that we've parsed all the input files, we should have the complete class heirarchy defined for each input
    // class, and can generate the output files.
    for (const auto& pair : classFiles) {
        std::ofstream outFile(pair.first);
        if (!outFile) {
            std::cerr << "Schema file create error on ouput file: " << pair.first << std::endl;
            return -1;
        }

        auto includeGuard = fmt::format("SRC_HADRON_SCHEMA_{:012X}", hadron::hash(pair.first));
        outFile << "#ifndef " << includeGuard << std::endl;
        outFile << "#define " << includeGuard << std::endl << std::endl;

        outFile << "// NOTE: schemac automatically generated this file from sclang input file." << std::endl;
        outFile << "// Edits will likely be clobbered." << std::endl << std::endl;

        outFile << "namespace hadron {" << std::endl;
        outFile << "namespace schema {" << std::endl << std::endl;

        for (const auto& className : pair.second) {
            auto classIter = classes.find(className);
            if (classIter == classes.end()) {
                std::cerr << "Mismatch between class name in file and class name in map: " << className << std::endl;
                return -1;
            }

            outFile << "// ========== " << className << std::endl;
            outFile << fmt::format("struct {}Schema {{\n", className);
            outFile << fmt::format("    static constexpr Hash kNameHash = 0x{:012x};\n", hadron::hash(className));
            outFile << fmt::format("    static constexpr Hash kMetaNameHash = 0x{:012x};\n",
                    hadron::hash(fmt::format("Meta_{}", className)));

            if (classIter->second.isPrimitiveType) {
                outFile << "};" << std::endl << std::endl;
                continue;
            }

            std::stack<std::unordered_map<std::string, ClassInfo>::iterator> lineage;
            auto lineageIter = classIter;
            lineage.emplace(lineageIter);

            while (lineageIter->second.superClassName != "") {
                lineageIter = classes.find(lineageIter->second.superClassName);
                if (lineageIter == classes.end()) {
                    std::cerr << "Missing class definition in lineage for " << className << std::endl;
                    return -1;
                }
                lineage.emplace(lineageIter);
            }

            outFile << std::endl << "    library::Schema schema;" << std::endl << std::endl;

            // Lineage in order from top to bottom.
            while (lineage.size()) {
                lineageIter = lineage.top();
                lineage.pop();
                outFile << "    // " << lineageIter->second.className << std::endl;
                for (const auto& varName : lineageIter->second.variables) {
                    outFile << "    Slot " << varName << ";" << std::endl;
                }
            }

            outFile << "};" << std::endl << std::endl;
            outFile << "static_assert(std::is_standard_layout<" << className << "Schema>::value);"
                    << std::endl << std::endl;
        }

        outFile << "} // namespace schema" << std::endl;
        outFile << "} // namespace hadron" << std::endl << std::endl;

        outFile << "#endif // " << includeGuard << std::endl;
    }

    return 0;
}