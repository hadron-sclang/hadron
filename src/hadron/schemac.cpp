// schemac, parses a SuperCollider input class file and generates a Schema C++ output file
#include "hadron/Hash.hpp"
#include "hadron/SourceFile.hpp"
#include "internal/FileSystem.hpp"

#include "antlr4-runtime.h"
#include "fmt/format.h"
#include "gflags/gflags.h"
#include "SCLexer.h"
#include "SCParser.h"
#include "SCParserBaseListener.h"
#include "SCParserListener.h"

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

class SchemaListener : public sprklr::SCParserBaseListener {
public:
    SchemaListener() = delete;
    explicit SchemaListener(std::unordered_map<std::string, ClassInfo>& classes):
        m_classes(classes),
        m_inClassVarDecl(false) {}
    virtual ~SchemaListener() {}

    void enterClassDef(sprklr::SCParser::ClassDefContext* ctx) override {
        auto className = ctx->CLASSNAME()->toString();
        m_classInfo.className = className;
        m_classNames.emplace_back(className);

        // Reset other ClassInfo fields, starting a new class.
        m_classInfo.superClassName = "";
        m_classInfo.isFundamentalType = FundamentalTypeNames.count(className) != 0;
        m_classInfo.variables.clear();
    }

    void enterSuperclass(sprklr::SCParser::SuperclassContext* ctx) override {
        m_classInfo.superClassName = ctx->CLASSNAME()->toString();
    }

    void enterClassVarDecl(sprklr::SCParser::ClassVarDeclContext* ctx) override {
        m_inClassVarDecl = ctx->VAR() != nullptr;
    }

    void enterName(sprklr::SCParser::NameContext* ctx) override {
        if (m_inClassVarDecl) {
            m_classInfo.variables.emplace_back(ctx->NAME()->toString());
        }
    }

    void exitClassVarDecl(sprklr::SCParser::ClassVarDeclContext* ctx) override {
        m_inClassVarDecl = false;
    }

    void exitClassDef(sprklr::SCParser::ClassDefContext* ctx) override {
        m_classes.emplace(std::make_pair(m_classInfo.className, std::move(m_classInfo)));
    }

    const std::vector<std::string>& classNames() const { return m_classNames; }

private:
    ClassInfo m_classInfo;
    std::unordered_map<std::string, ClassInfo>& m_classes;
    std::vector<std::string> m_classNames;
    bool m_inClassVarDecl;
};

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

    // Map of class names to info.
    std::unordered_map<std::string, ClassInfo> classes;
    // Map of paths to in-order class names to define.
    std::unordered_map<std::string, std::vector<std::string>> classFiles;

    for (const auto& ioPair : ioFiles) {
        auto classFile = fs::path(ioPair.first);
        auto schemaPath = fs::path(ioPair.second);

        hadron::SourceFile sourceFile(classFile);
        if (!sourceFile.read()) {
            std::cerr << "Failed to read input class file: " << classFile << std::endl;
            return -1;
        }
        auto code = sourceFile.codeView();

        // Drop the null character at the end of the string as ANTLR lexer chokes on it.
        antlr4::ANTLRInputStream input(code.data(), code.size() - 1);
        sprklr::SCLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        sprklr::SCParser parser(&tokens);
        auto parseTree = parser.root();

        auto listener = SchemaListener(classes);
        antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, parseTree);

        if (parser.getNumberOfSyntaxErrors()) {
            for (auto token : tokens.getTokens()) {
                std::cout << token->toString() << std::endl;
            }
            std::cerr << classFile << " had " << parser.getNumberOfSyntaxErrors() << " syntax errors.\n";
            return -1;
        }

        classFiles.emplace(std::make_pair(schemaPath, listener.classNames()));
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