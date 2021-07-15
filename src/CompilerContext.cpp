#include "hadron/CompilerContext.hpp"

#include "FileSystem.hpp"
#include "hadron/CodeGenerator.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/MachineCodeRenderer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "hadron/VirtualJIT.hpp"

#include <fstream>
#include <string_view>

namespace hadron {

CompilerContext::CompilerContext(std::unique_ptr<char[]> code, size_t codeSize):
    m_code(std::move(code)),
    m_codeSize(codeSize),
    m_errorReporter(std::make_unique<ErrorReporter>()) {
    m_errorReporter->setCode(m_code.get());
}

CompilerContext::CompilerContext(std::string filePath):
    m_filePath(filePath),
    m_errorReporter(std::make_unique<ErrorReporter>()) { }

CompilerContext::~CompilerContext() { }

bool CompilerContext::readFile() {
    fs::path filePath(m_filePath);
    if (!fs::exists(filePath)) {
        m_errorReporter->addFileNotFoundError(m_filePath);
        return false;
    }

    m_codeSize = fs::file_size(filePath);
    m_code = std::make_unique<char[]>(m_codeSize);
    std::ifstream inFile(filePath);
    if (!inFile) {
        m_errorReporter->addFileOpenError(filePath);
        return false;
    }
    inFile.read(m_code.get(), m_codeSize);
    if (!inFile) {
        m_errorReporter->addFileReadError(filePath);
        return false;
    }

    m_errorReporter->setCode(m_code.get());
    return true;
}

bool CompilerContext::lex() {
    m_lexer = std::make_unique<Lexer>(std::string_view(m_code.get(), m_codeSize), m_errorReporter);
    return m_lexer->lex();
}

bool CompilerContext::parse() {
    if (!m_errorReporter->ok()) {
        return false;
    }

    if (!m_lexer) {
        if (!lex()) {
            return false;
        }
    }
    m_parser = std::make_unique<Parser>(m_lexer.get(), m_errorReporter);
    return m_parser->parse();
}

bool CompilerContext::analyzeSyntax() {
    if (!m_errorReporter->ok()) {
        return false;
    }

    if (!m_parser) {
        if (!parse()) {
            return false;
        }
    }
    m_syntaxAnalyzer = std::make_unique<SyntaxAnalyzer>(m_parser.get(), m_errorReporter);
    return m_syntaxAnalyzer->buildAST();
}

bool CompilerContext::generateCode() {
    if (!m_errorReporter->ok()) {
        return false;
    }

    if (!m_syntaxAnalyzer) {
        if (!analyzeSyntax()) {
            return false;
        }
    }

    // Assumption right now is top of syntax tree is a block.
    m_codeGenerator = std::make_unique<CodeGenerator>(reinterpret_cast<const ast::BlockAST*>(m_syntaxAnalyzer->ast()),
        m_errorReporter);
    return m_codeGenerator->generate();
}

bool CompilerContext::renderToMachineCode() {
    return false;
}

bool CompilerContext::getGeneratedCodeAsString(std::string& codeString) const {
    if (!m_errorReporter->ok() || !m_codeGenerator) {
        return false;
    }

    return m_codeGenerator->virtualJIT()->toString(codeString);
}

} // namespace hadron