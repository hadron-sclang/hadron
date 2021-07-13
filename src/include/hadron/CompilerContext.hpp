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

// Owns the source code input and keeps all of the necessary components around for compilation of a piece of code
// from source to output C++ or JIT machine code.
class CompilerContext {
public:
    // Takes over ownership of |code|.
    CompilerContext(std::unique_ptr<char[]> code, size_t codeSize);
    explicit CompilerContext(std::string filePath);
    ~CompilerContext();

    bool readFile();

    // These are the compilation steps in order. Out-of-order calls are undefined. If a later order function is called
    // it will attempt to call the previous steps in order.
    bool lex();
    bool parse();
    bool analyzeSyntax();
    bool generateCode();
    bool renderToMachineCode();

    bool getGeneratedCodeAsString(std::string& codeString) const;


private:
    std::string m_filePath;

    std::unique_ptr<char[]> m_code;
    size_t m_codeSize;
    std::shared_ptr<ErrorReporter> m_errorReporter;

    std::unique_ptr<Lexer> m_lexer;
    std::unique_ptr<Parser> m_parser;
    std::unique_ptr<SyntaxAnalyzer> m_syntaxAnalyzer;
    std::unique_ptr<CodeGenerator> m_codeGenerator;
    std::unique_ptr<MachineCodeRenderer> m_codeRenderer;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_COMPILER_CONTEXT_HPP_