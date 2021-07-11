#ifndef SRC_LEXER_HPP_
#define SRC_LEXER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Literal.hpp"
#include "hadron/Token.hpp"
#include "hadron/Type.hpp"

#include <cstddef>
#include <memory>
#include <stdint.h>
#include <stdlib.h>
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

private:
    std::string_view m_code;
    std::vector<Token> m_tokens;

    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_LEXER_HPP_
