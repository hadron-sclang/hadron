#include "hadron/CompilerContext.hpp"

#include "hadron/CodeGenerator.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/SyntaxAnalyzer.hpp"

#include <string_view>

namespace hadron {

CompilerContext::CompilerContext(std::unique_ptr<char[]> code, size_t codeSize):
    m_code(std::move(code)),
    m_codeSize(codeSize),
    m_errorReporter(std::make_unique<ErrorReporter>()) {
    m_errorReporter->setCode(m_code.get());
}

bool CompilerContext::lex() {
    m_lexer = std::make_unique<Lexer>(std::string_view(m_code.get(), m_codeSize), m_errorReporter);
    return m_lexer->lex();
}

bool CompilerContext::parse() {
    if (!m_lexer) {
        if (!lex()) {
            return false;
        }
    }
    m_parser = std::make_unique<Parser>(m_lexer.get(), m_errorReporter);
    return m_parser->parse();
}

bool CompilerContext::analyzeSyntax() {
    if (!m_parser) {
        if (!parse()) {
            return false;
        }
    }
    m_syntaxAnalyzer = std::make_unique<SyntaxAnalyzer>(m_parser.get(), m_errorReporter);
    return m_syntaxAnalyzer->buildAST();
}

} // namespace hadron