#ifndef SRC_COMPILER_INCLUDE_HADRON_LEXER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LEXER_HPP_

#include "hadron/Token.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace hadron {

class ErrorReporter;

class Lexer {
public:
    // For testing, builds its own ErrorReporter
    Lexer(std::string_view code);
    Lexer(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter);

    ~Lexer() = default;

    bool lex();

    const std::vector<Token>& tokens() const { return m_tokens; }

    // Access for testing
    std::shared_ptr<ErrorReporter> errorReporter() { return m_errorReporter; }
    std::string_view code() const { return m_code; }

private:
    std::string_view m_code;
    std::vector<Token> m_tokens;

    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LEXER_HPP_
