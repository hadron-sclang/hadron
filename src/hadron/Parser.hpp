#ifndef SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/library/HadronParseNode.hpp"
#include "hadron/Slot.hpp"
#include "hadron/Token.hpp"

namespace hadron {

class ErrorReporter;
class Lexer;
struct ThreadContext;

class Parser {
public:
    // Builds a parse tree from an external lexer that has already successfully lexed the source code.
    Parser(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter);

    // Used for testing, lexes the code itself with an owned Lexer first.
    Parser(std::string_view code);
    ~Parser();

    // Parse interpreter input. root() will always be a BlockNode or an empty Node (in event of empty input)
    bool parse(ThreadContext* context);
    // Parse input with class definitions or class extensions. root() will be a ClassNode or ClassExtNode.
    bool parseClass(ThreadContext* context);

    const library::Node root() const { return m_root; }
    Lexer* lexer() const { return m_lexer; }
    std::shared_ptr<ErrorReporter> errorReporter() { return m_errorReporter; }

    // Access to parser from Bison Parser
    void addRoot(library::Node root);
    Token token(size_t index) const;
    size_t tokenIndex() const { return m_tokenIndex; }
    void next() { ++m_tokenIndex; }
    bool sendInterpret() const { return m_sendInterpret; }
    void setInterpret(bool i) { m_sendInterpret = i; }

private:
    bool innerParse(ThreadContext* context);

    std::unique_ptr<Lexer> m_ownLexer;
    Lexer* m_lexer;
    size_t m_tokenIndex;
    bool m_sendInterpret;

    std::shared_ptr<ErrorReporter> m_errorReporter;
    library::Node m_root;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_
