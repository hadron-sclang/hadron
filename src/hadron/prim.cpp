// prim, parses a SuperCollider input class file and produces primitive function signatures and routing fragments
#include "hadron/SourceFile.hpp"
#include "internal/FileSystem.hpp"

#include "fmt/format.h"
#include "gflags/gflags.h"
#include "SCLexer.h"
#include "SCParser.h"
#include "SCParserBaseListener.h"
#include "SCParserListener.h"

#include <fstream>
#include <string>
#include <unordered_set>

namespace {

// maybe we only output the case file? Like assume that the methods are already declared/defined in the relevant
// function headers. This gives us types on objects, btw.
// case 0xblahblah: { // _BasicNew
//   Meta_Object target(arg0);
//   Integer maxSize(arg0) <-- how do we know this?
//   Object result = target._BasicNew(context, maxSize); <-- how do we know return type?
//   context->framePointer->arg0 = result.slot();
// } break;

class PrimitiveListener : public sprklr::SCParserBaseListener {
public:
    PrimitiveListener() = delete;
    PrimitiveListener(std::ofstream& caseFile, std::ofstream& declFile):
        m_caseFile(caseFile),
        m_declFile(declFile),
        m_inPrimitive(false) {}
    virtual ~PrimitiveListener() {}

    void enterClassDef(sprklr::SCParser::ClassDefContext* ctx) override {
        m_className = ctx->CLASSNAME()->toString();
    }

    void enterMethodDef(sprklr::SCParser::MethodDefContext* ctx) override {
        if (!ctx->primitive())
            return;

        m_primitiveName = ctx->primitive()->PRIMITIVE()->toString();
        if (m_primitives.count(m_primitiveName)) {
            return;
        }
        m_primitives.insert(m_primitiveName);
        m_inPrimitive = true;
        // First argument is always;
        m_thisType = ctx->ASTERISK() ? fmt::format("Meta_{}", m_className) : m_className;
        m_signature = fmt::format("Slot {}(ThreadContext* context, {} this_", m_primitiveName, m_thisType);
    }

    void enterArgDecls(sprklr::SCParser::ArgDeclsContext* /* ctx */) override { }

    void enterPipeDef(sprklr::SCParser::PipeDefContext* ctx) override {
        if (!m_inPrimitive)
            return;

        m_signature += fmt::format(", Slot {}", ctx->name()->NAME()->toString());
    }

    void enterVarDef(sprklr::SCParser::VarDefContext* ctx) override {
        if (!m_inPrimitive)
            return;

        m_signature += fmt::format(", Slot {}", ctx->name()->NAME()->toString());
    }

    void exitArgDecls(sprklr::SCParser::ArgDeclsContext* /* ctx */) override {
        if (!m_inPrimitive)
            return;

        m_inPrimitive = false;
        m_signature += ");\n";
        m_declFile << m_signature;
    }


private:
    std::unordered_set<std::string> m_primitives;
    std::ofstream& m_caseFile;
    std::ofstream& m_declFile;

    std::string m_className;
    bool m_inPrimitive;
    std::string m_primitiveName;
    std::string m_thisType;
    std::string m_signature;
};

} // namespace

DEFINE_string(caseFile, "", "Output file name for case statements for primitive functions.");
DEFINE_string(declFile, "", "Output file name for primitive prototype declarations.");

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    if (argc != 2) {
        std::cerr << "usage: prim [options] input-file.sc\n";
        return -1;
    }

    auto sourcePath = std::string(argv[1]);
    hadron::SourceFile sourceFile(sourcePath);
    if (!sourceFile.read()) {
        std::cerr << "Failed to read input class file: " << sourcePath << "\n";
        return -1;
    }
    auto code = sourceFile.codeView();

    // Drop the null character at the end of the string as ANTLR lexer chokes on it.
    antlr4::ANTLRInputStream input(code.data(), code.size() - 1);
    sprklr::SCLexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    sprklr::SCParser parser(&tokens);
    auto parseTree = parser.root();

    std::ofstream caseFile(FLAGS_caseFile);
    if (!caseFile) {
        std::cerr << "Failed to open case file: " << FLAGS_caseFile << "\n";
        return -1;
    }
    std::ofstream declFile(FLAGS_declFile);
    if (!declFile) {
        std::cerr << "Failed to open decl file: " << FLAGS_declFile << "\n";
        return -1;
    }


    auto listener = PrimitiveListener(caseFile, declFile);
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, parseTree);



    return 0;
}