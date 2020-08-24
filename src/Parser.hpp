#ifndef SRC_PARSER_HPP_
#define SRC_PARSER_HPP_

#include <memory>
#include <vector>

namespace hadron {

namespace parse {

class Node {
public:
    Node();
    virtual ~Node();

private:
};

} // namespace parse

class Parser {
public:
    Parser(std::shared_ptr<Lexer> lexer);
    ~Parser();

    bool parse();

private:
    std::shared_ptr<Lexer> m_lexer;
    std::unique_ptr<parse::Node> m_root;
};

} // namespace hadron

#endif // SRC_PARSER_HPP_
