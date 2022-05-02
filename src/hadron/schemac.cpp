// schema, parses a SuperCollider input class file and generates a Schema C++ output file
#include "hadron/ErrorReporter.hpp"
#include "hadron/Hash.hpp"
#include "hadron/Lexer.hpp"
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

bool processPaths(const std::string& inputFiles, const std::string& basePath, const std::string& schemaBasePath,
        std::unordered_map<std::string, std::string>& ioFiles) {
    // Parse semicolon-delinated list of input class files.
    size_t pathBegin = 0;
    size_t pathEnd = inputFiles.find_first_of(';');
    do {
        fs::path classFile = pathEnd == std::string::npos ? inputFiles.substr(pathBegin) :
                inputFiles.substr(pathBegin, pathEnd - pathBegin);
        classFile = fs::absolute(classFile);

        if (!fs::exists(classFile)) {
            std::cerr << "Class file path: " << classFile.string() << " does not exist.";
            return false;
        }

        // The class file must be in a subdirectory of the library path.
        if (classFile.string().substr(0, basePath.length()) != basePath) {
            std::cerr << "Class file path: " << classFile.string() << " not in a subdirectory of library path: "
                    << basePath << std::endl;
            return false;
        }

        fs::path schemaPath = schemaBasePath /
            fs::path(classFile.string().substr(basePath.length())).parent_path().relative_path() /
            fs::path(classFile.stem().string() + "Schema.hpp");

        ioFiles.emplace(std::make_pair(classFile.string(), schemaPath.string()));

        if (pathEnd == std::string::npos) { break; }
        pathBegin = pathEnd + 1;
        pathEnd = inputFiles.find_first_of(';', pathBegin);
    } while (true);

    return true;
}

} // namespace

DEFINE_string(classFiles, "", "Semicolon-delineated list of input class files to process.");
DEFINE_string(libraryPath, "", "Base path of the SC class library.");
DEFINE_string(hlangFiles, "", "Semicolon-delineated list of input hlang class files to process.");
DEFINE_string(hlangPath, "", "Path to the HLang class library.");
DEFINE_string(schemaPath, "", "Base path of output schema files.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, false);

    fs::path libraryPath(FLAGS_libraryPath);
    if (!fs::exists(libraryPath)) {
        std::cerr << "Class library path does not exist: " << libraryPath << std::endl;
        return -1;
    }
    libraryPath = fs::absolute(libraryPath);

    fs::path hlangPath(FLAGS_hlangPath);
    if (!fs::exists(hlangPath)) {
        std::cerr << "HLang library path does not exist: " << hlangPath << std::endl;
        return -1;
    }
    hlangPath = fs::absolute(hlangPath);

    fs::path schemaBasePath(FLAGS_schemaPath);
    schemaBasePath = fs::absolute(schemaBasePath);

    // Map of input file path to output file path.
    std::unordered_map<std::string, std::string> ioFiles;

    if (!processPaths(FLAGS_classFiles, libraryPath, schemaBasePath, ioFiles)) {
        return -1;
    }
    if (!processPaths(FLAGS_hlangFiles, hlangPath, schemaBasePath, ioFiles)) {
        return -1;
    }

    auto errorReporter = std::make_shared<hadron::ErrorReporter>();
    // Map of class names to info.
    std::unordered_map<std::string, ClassInfo> classes;
    // Map of paths to in-order class names to define.
    std::unordered_map<std::string, std::vector<std::string>> classFiles;

    for (const auto& ioPair : ioFiles) {
        auto classFile = fs::path(ioPair.first);
        auto schemaPath = fs::path(ioPair.second);

        hadron::SourceFile sourceFile(classFile);
        if (!sourceFile.read(errorReporter)) {
            std::cerr << "Failed to read input class file: " << classFile << std::endl;
            return -1;
        }
        auto code = sourceFile.codeView();
        errorReporter->setCode(code.data());

        hadron::Lexer lexer(code, errorReporter);
        if (!lexer.lex() || !errorReporter->ok()) {
            std::cerr << "Failed to lex input class file: " << classFile << std::endl;
            return -1;
        }

        // Place an empty vector for appending class names.
        std::vector<std::string> classNames;

        enum State {
            kOutsideClass,
            kInClassExtension,
            kInClass,
            kInMethod
        };
        State scannerState = State::kOutsideClass;
        ClassInfo classInfo;
        int curlyBraceDepth = 0;

        for (size_t i = 0; i < lexer.tokens().size(); ++i) {
            const auto& token = lexer.tokens()[i];
            bool lastToken = (i == lexer.tokens().size() - 1);

            switch (scannerState) {

            case kOutsideClass: {
                assert(curlyBraceDepth == 0);
                if (lastToken) {
                    std::cerr << "Dangling token outside of class" << std::endl;
                    return -1;
                }

                // If outside a class the only valid tokens should be class names or a '+' indicating a class extension.
                if (token.name == hadron::Token::Name::kClassName) {
                    classInfo.className = std::string(token.range);
                    ++i;  // Advance past class name.

                    // Skip over the array declaration if present.
                    if (lexer.tokens()[i].name == hadron::Token::Name::kOpenSquare) {
                        ++i; // past open square.
                        if (lexer.tokens()[i].name == hadron::Token::Name::kIdentifier) {
                            ++i; // skip past optional name if present
                        }
                        if (lexer.tokens()[i].name != hadron::Token::Name::kCloseSquare) {
                            std::cerr << "Encountered open square bracket after class name but no closing bracket."
                                      << std::endl;
                            return -1;
                        }
                        ++i; // past closing square bracket
                    }

                    // Look for optional superclass name indicator, a colon.
                    if (lexer.tokens()[i].name == hadron::Token::Name::kColon) {
                        ++i; // skip past colon
                        if (lexer.tokens()[i].name != hadron::Token::Name::kClassName) {
                            std::cerr << "Expecting class name after colon." << std::endl;
                            return -1;
                        }
                        classInfo.superClassName = std::string(lexer.tokens()[i].name);
                    } else if (classInfo.className.compare("Object") != 0) {
                        classInfo.superClassName = std::string("Object");
                    }

                    if (lexer.tokens()[i].name != hadron::Token::Name::kOpenCurly) {
                        std::cerr << "Expecting opening curly brace after class definition." << std::endl;
                        return -1;
                    }
                    ++i;

                    scannerState = kInClass;
                } else if (token.name == hadron::Token::Name::kPlus) {
                    ++i; // skip over plus
                    if (lexer.tokens()[i].name != hadron::Token::Name::kClassName) {
                        std::cerr << "Encountered '+' symbol outside of class, but not followed by class name."
                                  << std::endl;
                        return -1;
                    }
                    ++i; // skip over the class name, not needed
                    classInfo.className = "";

                    if (lexer.tokens()[i].name != hadron::Token::Name::kOpenCurly) {
                        std::cerr << "Expecting open curly brace in class extension." << std::endl;
                        return -1;
                    }
                    ++i;
                    scannerState = kInClassExtension;
                } else {
                    std::cerr << "Unexpected token outside of class definition." << std::endl;
                    return -1;
                }
            } break;

            case kInClassExtension: {
                // Ignore all the tokens until the closing brace of the class extension.
                if (token.name == hadron::Token::Name::kOpenParen ||
                    token.name == hadron::Token::Name::kBeginClosedFunction) {
                    ++curlyBraceDepth;
                } else if (token.name == hadron::Token::Name::kCloseCurly) {
                    --curlyBraceDepth;
                }

                assert(curlyBraceDepth >= 0);
                if (curlyBraceDepth == 0) {
                    scannerState = kOutsideClass;
                }
            } break;

            case kInClass: {
                // Could be a 'var', 'const', 'classvar', '*' (for class methods), 'name', binop
                if (token.name == hadron::Token::Name::kVar) {
                    // pattern is an optional r/w/rw tag, identifier name, optional equal sign followed by anything
                    // up until a comma or semicolon. If a comma we start over, if a semicolon we're done.
                    do {
                        ++i; // var or ','

                        // skip optional rw spec
                        if (lexer.tokens()[i].name == hadron::Token::Name::kLessThan ||
                            lexer.tokens()[i].name == hadron::Token::Name::kGreaterThan ||
                            lexer.tokens()[i].name == hadron::Token::Name::kReadWriteVar) { ++i; }

                        if (lexer.tokens()[i].name != hadron::Token::Name::kIdentifier) {
                            std::cerr << "Expecting variable name." << std::endl;
                            return -1;
                        }
                        std::string varName(lexer.tokens()[i].range);
                        auto subs = keywordSubs.find(varName);
                        if (subs != keywordSubs.end()) { varName = subs->second; }
                        classInfo.variables.emplace_back(varName);
                        ++i; // identifier

                        // optional equal sign
                        if (lexer.tokens()[i].name == hadron::Token::Name::kAssign) {
                            ++i;
                            while (lexer.tokens()[i].name != hadron::Token::Name::kComma &&
                                   lexer.tokens()[i].name != hadron::Token::Name::kSemicolon) { ++i; }
                        }
                    } while (lexer.tokens()[i].name == hadron::Token::Name::kComma);

                    // Semicolon.
                    if (lexer.tokens()[i].name != hadron::Token::Name::kSemicolon) {
                        std::cerr << "Expecting semicolon at end of var list." << std::endl;
                        return -1;
                    }

                    ++i; // semicolon.
                } else if (token.name == hadron::Token::Name::kConst) {
                    while (lexer.tokens()[i].name != hadron::Token::kSemicolon) { ++i; }
                } else if (token.name == hadron::Token::Name::kClassVar) {
                    while (lexer.tokens()[i].name != hadron::Token::kSemicolon) { ++i; }
                } else if (token.name == hadron::Token::Name::kAsterisk) {

                } else if (token.name == hadron::Token::Name::kIdentifier || token.couldBeBinop) {
                    ++i; // pass identifier
                    if (token.name != hadron::Token::Name::kOpenCurly) {
                        std::cerr << "Expecting opening curly brace after method identifer." << std::endl;
                        return -1;
                    }
                } else {
                    std::cerr << "Unexpected token inside class." << std::endl;
                    return -1;
                }
            } break;

            case kInMethod: {

            } break;
            }
        }
/*
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
                if (lexer.tokens()[varList->tokenIndex].name == hadron::Token::Name::kVar) {
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
    }
*/
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