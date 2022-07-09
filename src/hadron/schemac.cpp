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
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
// While we generate a Schema struct for these objects they are not represented by Hadron with pointers, rather their
// values are packed into the Slot directly. So they are excluded from the Schema class heirarchy.
std::unordered_set<std::string> FundamentalTypeNames {
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
    bool isFundamentalType;
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
DEFINE_string(bootstrapPath, "", "Path to the class library bootstrap code output file.");

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
            kOutsideClass = 0,
            kInClassExtension = 1,
            kInClass = 2,
            kInMethod = 3
        };
        State scannerState = State::kOutsideClass;
        ClassInfo classInfo;
        int curlyBraceDepth = 0;
        const auto& tokens = lexer.tokens();

        for (size_t i = 0; i < tokens.size(); ++i) {
            bool lastToken = (i == tokens.size() - 1);

            switch (scannerState) {

            case kOutsideClass: {
                assert(curlyBraceDepth == 0);
                if (lastToken) {
                    std::cerr << "Dangling token outside of class" << std::endl;
                    return -1;
                }

                // If outside a class the only valid tokens should be class names or a '+' indicating a class extension.
                if (tokens[i].name == hadron::Token::Name::kClassName) {
                    classInfo.className = std::string(tokens[i].range);
                    classInfo.isFundamentalType = FundamentalTypeNames.count(classInfo.className) != 0;
                    classNames.emplace_back(classInfo.className);

                    ++i;  // Advance past class name.

                    // Skip over the array declaration if present.
                    if (tokens[i].name == hadron::Token::Name::kOpenSquare) {
                        ++i; // past open square.
                        if (tokens[i].name == hadron::Token::Name::kIdentifier) {
                            ++i; // skip past optional name if present
                        }
                        if (tokens[i].name != hadron::Token::Name::kCloseSquare) {
                            std::cerr << "Encountered open square bracket after class name but no closing bracket."
                                      << std::endl;
                            return -1;
                        }
                        ++i; // pass closing square bracket
                    }

                    // Look for optional superclass name indicator, a colon.
                    if (tokens[i].name == hadron::Token::Name::kColon) {
                        ++i; // skip past colon
                        if (tokens[i].name != hadron::Token::Name::kClassName) {
                            std::cerr << "Expecting class name after colon." << std::endl;
                            return -1;
                        }
                        classInfo.superClassName = std::string(tokens[i].range);
                        ++i; // pass superclass name
                    } else if (classInfo.className.compare("Object") != 0) {
                        classInfo.superClassName = std::string("Object");
                    }

                    if (tokens[i].name != hadron::Token::Name::kOpenCurly) {
                        std::cerr << "Expecting opening curly brace after class definition." << std::endl;
                        return -1;
                    }

                    curlyBraceDepth = 1;
                    scannerState = kInClass;
                } else if (tokens[i].name == hadron::Token::Name::kPlus) {
                    ++i; // skip over plus
                    if (tokens[i].name != hadron::Token::Name::kClassName) {
                        std::cerr << "Encountered '+' symbol outside of class, but not followed by class name."
                                  << std::endl;
                        return -1;
                    }
                    ++i; // skip over the class name, not needed
                    classInfo.className = "";

                    if (tokens[i].name != hadron::Token::Name::kOpenCurly) {
                        std::cerr << "Expecting open curly brace in class extension." << std::endl;
                        return -1;
                    }

                    curlyBraceDepth = 1;
                    scannerState = kInClassExtension;
                } else {
                    std::cerr << "Unexpected token " << tokens[i].name << " outside of class definition." << std::endl;
                    return -1;
                }
            } break;

            case kInClassExtension: {
                // Ignore all the tokens until the closing brace of the class extension.
                if (tokens[i].name == hadron::Token::Name::kOpenCurly ||
                    tokens[i].name == hadron::Token::Name::kBeginClosedFunction) {
                    ++curlyBraceDepth;
                } else if (tokens[i].name == hadron::Token::Name::kCloseCurly) {
                    --curlyBraceDepth;
                }

                assert(curlyBraceDepth >= 0);
                if (curlyBraceDepth == 0) {
                    scannerState = kOutsideClass;
                }
            } break;

            case kInClass: {
                assert(curlyBraceDepth == 1);

                // Could be a 'var', 'const', 'classvar', '*' (for class methods), 'name', binop
                if (tokens[i].name == hadron::Token::Name::kVar) {
                    // pattern is an optional r/w/rw tag, identifier name, optional equal sign followed by anything
                    // up until a comma or semicolon. If a comma we start over, if a semicolon we're done.
                    do {
                        ++i; // var or ','

                        // skip optional rw spec
                        if (tokens[i].name == hadron::Token::Name::kLessThan ||
                            tokens[i].name == hadron::Token::Name::kGreaterThan ||
                            tokens[i].name == hadron::Token::Name::kReadWriteVar) { ++i; }

                        if (tokens[i].name != hadron::Token::Name::kIdentifier) {
                            std::cerr << "Expecting variable name." << std::endl;
                            return -1;
                        }
                        std::string varName(tokens[i].range);
                        auto subs = keywordSubs.find(varName);
                        if (subs != keywordSubs.end()) { varName = subs->second; }
                        classInfo.variables.emplace_back(varName);
                        ++i; // identifier

                        // optional equal sign
                        if (tokens[i].name == hadron::Token::Name::kAssign) {
                            ++i; // pass equal sign
                            while (tokens[i].name != hadron::Token::Name::kComma &&
                                   tokens[i].name != hadron::Token::Name::kSemicolon) { ++i; }
                        }
                    } while (tokens[i].name == hadron::Token::Name::kComma);

                    // Semicolon to end the var statement.
                    if (tokens[i].name != hadron::Token::Name::kSemicolon) {
                        std::cerr << "Expecting semicolon at end of var list." << std::endl;
                        return -1;
                    }
                } else if (tokens[i].name == hadron::Token::Name::kConst) {
                    while (tokens[i].name != hadron::Token::kSemicolon) { ++i; }
                } else if (tokens[i].name == hadron::Token::Name::kClassVar) {
                    while (tokens[i].name != hadron::Token::kSemicolon) { ++i; }
                } else if (tokens[i].name == hadron::Token::Name::kAsterisk) {
                    ++i; // pass class method indicator.

                    // Asterisk can also be the name of the method, so skip the method name if present.
                    if (tokens[i].name == hadron::Token::Name::kIdentifier || tokens[i].couldBeBinop) {
                        ++i; // pass method name.
                    }

                    if (tokens[i].name != hadron::Token::Name::kOpenCurly) {
                        std::cerr << "Expecting opening curly brace after class method identifier." << std::endl;
                        return -1;
                    }

                    scannerState = kInMethod;
                    curlyBraceDepth = 2;
                } else if (tokens[i].name == hadron::Token::Name::kIdentifier || tokens[i].couldBeBinop) {
                    ++i; // pass identifier

                    if (tokens[i].name != hadron::Token::Name::kOpenCurly) {
                        std::cerr << "Expecting opening curly brace after method identifer." << std::endl;
                        return -1;
                    }
                    scannerState = kInMethod;
                    curlyBraceDepth = 2;
                } else if (tokens[i].name == hadron::Token::Name::kCloseCurly) {
                    // finished with class.
                    curlyBraceDepth = 0;
                    classes.emplace(std::make_pair(classInfo.className, std::move(classInfo)));
                    scannerState = kOutsideClass;
                } else {
                    std::cerr << "Unexpected token " << tokens[i].name << " inside class " << classInfo.className
                              << "." << std::endl;
                    return -1;
                }
            } break;

            case kInMethod: {
                // Ignore all the tokens until the closing brace of the class extension.
                if (tokens[i].name == hadron::Token::Name::kOpenCurly ||
                    tokens[i].name == hadron::Token::Name::kBeginClosedFunction) {
                    ++curlyBraceDepth;
                } else if (tokens[i].name == hadron::Token::Name::kCloseCurly) {
                    --curlyBraceDepth;
                }

                assert(curlyBraceDepth >= 1);
                if (curlyBraceDepth == 1) {
                    scannerState = kInClass;
                }
            } break;
            }
        }

        classFiles.emplace(std::make_pair(schemaPath, std::move(classNames)));
    }

    std::ofstream bootstrapFile(FLAGS_bootstrapPath);
    if (!bootstrapFile) {
        std::cerr << "Schema failed to create the bootstrap output file: " << FLAGS_bootstrapPath << std::endl;
        return -1;
    }
    bootstrapFile << "    library::Symbol className;\n"
                     "    library::Class classDef;\n"
                     "    library::SymbolArray instVarNames;\n";

    // Now that we've parsed all the input files, we should have the complete class heirarchy defined for each input
    // class, and can generate the output files.
    for (const auto& pair : classFiles) {
        std::ofstream outFile(pair.first);
        if (!outFile) {
            std::cerr << "Schema file create error on ouput file: " << pair.first << std::endl;
            return -1;
        }

        auto includeGuard = fmt::format("SRC_HADRON_SCHEMA_{:08X}", hadron::hash(pair.first));
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
            outFile << fmt::format("    static constexpr Hash kNameHash = 0x{:08x};\n", hadron::hash(className));
            outFile << fmt::format("    static constexpr Hash kMetaNameHash = 0x{:08x};\n",
                    hadron::hash(fmt::format("Meta_{}", className)));

            if (classIter->second.isFundamentalType) {
                outFile << "};" << std::endl << std::endl;
                continue;
            }

            bootstrapFile << "\n    // ========== " << className
                    << fmt::format("\n    className = library::Symbol::fromView(context, \"{}\");\n", className)
                    << "    m_bootstrapClasses.emplace(className);\n"
                    << "    classDef = findOrInitClass(context, className);\n"
                    << "    instVarNames = library::SymbolArray::arrayAlloc(context);\n";

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
                auto classHeader = fmt::format("    // {}\n", lineageIter->second.className);
                outFile << classHeader;
                bootstrapFile << classHeader;
                for (const auto& varName : lineageIter->second.variables) {
                    outFile << "    Slot " << varName << ";" << std::endl;
                    bootstrapFile <<
                            fmt::format("    instVarNames = instVarNames.add(context, library::Symbol::fromView("
                                    "context, \"{}\"));\n", varName);
                }
            }

            outFile << "};" << std::endl << std::endl;
            outFile << "static_assert(std::is_standard_layout<" << className << "Schema>::value);"
                    << std::endl << std::endl;

            bootstrapFile << "    classDef.setInstVarNames(instVarNames);\n";
        }

        outFile << "} // namespace schema" << std::endl;
        outFile << "} // namespace hadron" << std::endl << std::endl;

        outFile << "#endif // " << includeGuard << std::endl;
    }

    return 0;
}