#ifndef SRC_INCLUDE_HADRON_COMPILER_CONTEXT_HPP_
#define SRC_INCLUDE_HADRON_COMPILER_CONTEXT_HPP_

#include <memory>
#include <string>

namespace hadron {

class CodeGenerator;
class ErrorReporter;
class Lexer;
class MachineCodeRenderer;
class Parser;
class SyntaxAnalyzer;
struct Slot;

// Owns the source code input and keeps all of the necessary components around for compilation of a piece of code
// from source to output C++ or JIT machine code.
class CompilerContext {
public:
    // Takes over ownership of |code|.
    CompilerContext(std::unique_ptr<char[]> code, size_t codeSize);
    explicit CompilerContext(std::string filePath);
    ~CompilerContext();

    // Some compiler tools have global static data they need to set up. Call these exactly once per process, before
    // using this class.
    static void initJITGlobals();
    static void finishJITGlobals();

    bool readFile();

    // These are the compilation steps in order. Out-of-order calls are undefined. If a later order function is called
    // it will attempt to call the previous steps in order.
    bool lex();
    bool parse();
    bool analyzeSyntax();
    bool generateCode();
    bool renderToMachineCode();

    // JIT, then run the generated code, save the result in |value|, returns true on success.
    bool evaluate(Slot* value);
    bool getGeneratedCodeAsString(std::string& codeString) const;

private:
    std::string m_filePath;

    std::unique_ptr<char[]> m_code;
    size_t m_codeSize;
    std::shared_ptr<ErrorReporter> m_errorReporter;

    std::unique_ptr<Lexer> m_lexer;
    std::unique_ptr<Parser> m_parser;
    std::unique_ptr<SyntaxAnalyzer> m_syntaxAnalyzer;
    std::unique_ptr<CodeGenerator> m_generator;
    std::unique_ptr<MachineCodeRenderer> m_renderer;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_COMPILER_CONTEXT_HPP_