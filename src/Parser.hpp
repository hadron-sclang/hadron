#ifndef SRC_PARSER_HPP_
#define SRC_PARSER_HPP_

#include "Lexer.hpp"

#include <memory>
#include <string_view>

namespace hadron {

class ErrorReporter;

namespace parse {
class Node;
} // namespace parse

class Parser {
public:
    Parser(const char* code, std::shared_ptr<ErrorReporter> errorReporter);
    ~Parser();

    bool parse();

private:
    std::unique_ptr<Lexer> m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<parse::Node> m_root;
};

namespace parse {

struct Node {
    Node(): tail(this) {}
    virtual ~Node() = default;
    void append(std::unique_ptr<Node> node) {
        tail->next = std::move(node);
        tail = node.get();
    }

    std::unique_ptr<Node> next;
    Node* tail;
};

struct MethodNode : public Node {
    MethodNode(std::string_view name): methodName(name) {}
    virtual ~MethodNode() = default;

    std::string_view methodName;
};

struct ClassExtNode : public Node {
    ClassExtNode(std::string_view name): className(name) {}
    virtual ~ClassExtNode() = default;

    std::string_view className;
    std::unique_ptr<MethodNode> methods;
};

struct SlotNode : public Node {
    SlotNode();
    virtual ~SlotNode() = default;
};

struct VarDefNode : public Node {
    VarDefNode();
    virtual ~VarDefNode() = default;
};

struct VarListNode : public Node {
    VarListNode();
    virtual ~VarListNode() = default;

    std::unique_ptr<VarDefNode> definitions;
};

struct ClassNode : public Node {
    ClassNode(std::string_view name): className(name) {}
    virtual ~ClassNode() = default;

    std::string_view className;
    std::string_view superClassName;
    std::string_view optionalName;

    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<MethodNode> methods;
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

struct BlockNode : public Node {
    BlockNode();
    virtual ~BlockNode() = default;

    std::unique_ptr<ArgListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<Node> body;
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
