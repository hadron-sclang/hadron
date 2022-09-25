#ifndef SRC_COMPILER_INCLUDE_HADRON_LEXER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_LEXER_HPP_

#include "hadron/Token.hpp"

#include <memory>
#include <set>
#include <string_view>
#include <vector>

namespace hadron {

class ErrorReporter;

class Lexer {
public:
    Lexer(std::string_view code);

    ~Lexer() = default;

    bool lex();

    const std::vector<Token>& tokens() const { return m_tokens; }

    // Access for testing
    std::string_view code() const { return m_code; }

private:
    Token::Location getLocation(const char* p);

    std::string_view m_code;
    std::vector<Token> m_tokens;
    std::set<int32_t> m_lineEndings;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_LEXER_HPP_
