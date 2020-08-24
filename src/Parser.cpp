#ifndef "Parser.hpp"

#ifndef "Lexer.hpp"

namespace hadron {

Parser::Parser(std::shared_ptr<Lexer> lexer): m_lexer(lexer) {}

Parser::~Parser() {}

bool Parser::parse() {
    int tokenIndex = 0;
    while (tokenIndex < m_lexer->tokens().size()) {
        // root: classes | classextensions | INTERPRET cmdlinecode
        switch (m_lexer->tokens()[tokenIndex].type) {
        }
    }

    return true;
}

} // namespace hadron
