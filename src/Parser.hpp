#ifndef SRC_PARSER_HPP_
#define SRC_PARSER_HPP_

#include "Lexer.hpp"

#include <memory>
#include <vector>

namespace hadron {

namespace parse {
class Node;
} // namespace parse

class Parser {
public:
    Parser(const char* code);
    ~Parser();

    bool parse();

private:
    std::unique_ptr<Lexer> m_lexer;
    std::unique_ptr<parse::Node> m_root;
    bool m_parseOK;
};

namespace parse {

struct Node {
    Node();
    virtual ~Node() = default;
};

struct BlockNode : public Node {
    BlockNode();
    virtual ~BlockNode() = default;

    std::vector<std::unique_ptr<Node>> variables;
    std::vector<std::unique_ptr<Node>> body;
};

} // namespace parse

} // namespace hadron

#endif // SRC_PARSER_HPP_
