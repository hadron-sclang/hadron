#include "Parser.hpp"

#include "Lexer.hpp"

namespace {

std::unique_ptr<hadron::parse::VarDefNode> parseVarDef(hadron::Lexer* /* lexer */) {
    return nullptr;
}

bool parseFunctionBody(hadron::Lexer* /* lexer */, std::vector<std::unique_ptr<hadron::parse::Node>>& /* body */) {
    return false;
}

// root: classes | classextensions | INTERPRET cmdlinecode
std::unique_ptr<hadron::parse::Node> parseRoot(hadron::Lexer* lexer) {
    if (lexer->next()) {
        // TODO: classes
        // TODO: classextensions

        // cmdlinecode: '(' funcvardecls1 funcbody ')'
        //              | funcvardecls1 funcbody
        //              | funcbody
        bool gotParen = lexer->token().type == hadron::Lexer::Token::Type::kOpenParen;
        if (gotParen) {
            // Swallow the opening parenthesis if it was there.
            if (!lexer->next()) {
                return nullptr;
            }
        }

        auto block = std::make_unique<hadron::parse::BlockNode>();

        // funcvardecls: <e> | funcvardecls funcvardecl
        // funcvardecls1: funcvardecl | funcvardecls1 funcvardecl
        // funcvardecl: VAR vardeflist ';'
        // (reads as parse 0 or more VAR statements)
        while (lexer->token().type == hadron::Lexer::Token::Type::kVar) {
            // swallow VAR
            if (!lexer->next()) {
                return nullptr;
            }
            std::unique_ptr<hadron::parse::VarDefNode> varDef = parseVarDef(lexer);
            if (!varDef) {
                return nullptr;
            }
            block->variables.emplace_back(std::move(varDef));
        }

        if (!parseFunctionBody(lexer, block->body)) {
            return nullptr;
        }

        if (gotParen) {
            if (!lexer->next() || lexer->token().type == hadron::Lexer::Token::Type::kCloseParen) {
                return nullptr;
            }
        }
        return block;
    }

    return nullptr;
}

}

namespace hadron {

Parser::Parser(const char* code): m_lexer(std::make_unique<Lexer>(code)), m_parseOK(false) {}

Parser::~Parser() {}

bool Parser::parse() {
    m_root = parseRoot(m_lexer.get());
    return m_root != nullptr;
}

} // namespace hadron
