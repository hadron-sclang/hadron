#ifndef SRC_PARSER_HPP_
#define SRC_PARSER_HPP_

#include "Lexer.hpp"

#include <list>
#include <memory>

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

    std::list<std::unique_ptr<Node>> siblings;
};

struct ClassNode : public Node {
    ClassNode();
    virtual ~ClassNode() = default;
};

struct ClassExtNode : public Node {
    ClassExtNode();
    virtual ~ClassExtNode() = default;
};

struct MethodNode : public Node {
    MethodNode();
    virtual ~MethodNode() = default;
};

struct BlockNode : public Node {
    BlockNode();
    virtual ~BlockNode() = default;
};

struct SlotNode : public Node {
    SlotNode();
    virtual ~SlotNode() = default;
};

struct VarListNode : public Node {
    VarListNode();
    virtual ~VarListNode() = default;
};

struct VarDefNode : public Node {
    VarDefNode();
    virtual ~VarDefNode() = default;
};

struct DynDictNode : public Node {
    DynDictNode();
    virtual ~DynDictNode() = default;
};

struct DynListNode : public Node {
    DynListNode();
    virtual ~DynListNode() = default;
};

struct LitListNode : public Node {
    LitListNode();
    virtual ~LitListNode() = default;
};

struct LitDictNode : public Node {
    LitDictNode();
    virtual ~LitDictNode() = default;
};

struct StaticVarListNode : public Node {
    StaticVarListNode();
    virtual ~StaticVarListNode() = default;
};

struct InstVarListNode : public Node {
    InstVarListNode();
    virtual ~InstVarListNode() = default;
};

struct PoolVarListNode : public Node {
    PoolVarListNode();
    virtual ~PoolVarListNode() = default;
};

struct ArgListNode : public Node {
    ArgListNode();
    virtual ~ArgListNode() = default;
};

struct SlotDefNode : public Node {
    SlotDefNode();
    virtual ~SlotDefNode() = default;
};

struct LiteralNode : public Node {
    LiteralNode();
    virtual ~LiteralNode() = default;
};

struct PushLiteralNode : public Node {
    PushLiteralNode();
    virtual ~PushLiteralNode() = default;
};

struct PushNameNode : public Node {
    PushNameNode();
    virtual ~PushNameNode() = default;
};

struct CallNode : public Node {
    CallNode();
    virtual ~CallNode() = default;
};

struct BinopCallNode : public Node {
    BinopCallNode();
    virtual ~BinopCallNode() = default;
};

struct DropNode : public Node {
    DropNode();
    virtual ~DropNode() = default;
};

struct AssignNode : public Node {
    AssignNode();
    virtual ~AssignNode() = default;
};

struct MultiAssignNode : public Node {
    MultiAssignNode();
    virtual ~MultiAssignNode() = default;
};

struct SetterNode : public Node {
    SetterNode();
    virtual ~SetterNode() = default;
};

struct CurryArgNode : public Node {
    CurryArgNode();
    virtual ~CurryArgNode() = default;
};

struct ReturnNode : public Node {
    ReturnNode();
    virtual ~ReturnNode() = default;
};

struct BlockReturnNode : public Node {
    BlockReturnNode();
    virtual ~BlockReturnNode() = default;
};

} // namespace parse
} // namespace hadron

#endif // SRC_PARSER_HPP_
