#include "SyntaxAnalyzer.hpp"

#include "Parser.hpp"

namespace hadron {

SyntaxAnalyzer::SyntaxAnalyzer(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) {}

bool SyntaxAnalyzer::buildAST(const parse::Node* /* root */) {
    return false;
}

} // namespace hadron