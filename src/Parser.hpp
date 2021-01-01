#ifndef SRC_PARSER_HPP_
#define SRC_PARSER_HPP_

#include "Lexer.hpp"
#include "TypedValue.hpp"

#include <memory>
#include <string_view>

namespace hadron {

class ErrorReporter;

namespace parse {

enum NodeType {
    kEmpty, // represents an empty node
    kVarDef,
    kVarList,
    kArgList,
    kMethod,
    kClassExt,
    kClass,
    kReturn,
    kBlock,
    kValue,
    kLiteral,
    kName
};

struct Node {
    Node() = delete;
    explicit Node(NodeType type, size_t index): nodeType(type), tokenIndex(index), tail(this) {}
    virtual ~Node() = default;
    void append(std::unique_ptr<Node> node) {
        tail->next = std::move(node);
        tail = node.get();
    }

    NodeType nodeType;
    size_t tokenIndex;
    std::unique_ptr<Node> next;
    Node* tail;
};

struct VarDefNode : public Node {
    VarDefNode(size_t index, std::string_view name): Node(NodeType::kVarDef, index), varName(name) {}
    virtual ~VarDefNode() = default;

    std::string_view varName;
    std::unique_ptr<Node> initialValue;
};

struct VarListNode : public Node {
    VarListNode(size_t index): Node(NodeType::kVarList, index) {}
    virtual ~VarListNode() = default;

    std::unique_ptr<VarDefNode> definitions;
};

struct ArgListNode : public Node {
    ArgListNode(size_t index): Node(NodeType::kArgList, index) {}
    virtual ~ArgListNode() = default;

    std::unique_ptr<VarListNode> varList;
    std::string_view varArgsName;
};

struct MethodNode : public Node {
    MethodNode(size_t index, std::string_view name, bool classMethod):
            Node(NodeType::kMethod, index),
            methodName(name),
            isClassMethod(classMethod) {}
    virtual ~MethodNode() = default;

    std::string_view methodName;
    bool isClassMethod;

    std::unique_ptr<ArgListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<Node> body;
};

struct ClassExtNode : public Node {
    ClassExtNode(size_t index, std::string_view name): Node(NodeType::kClassExt, index), className(name) {}
    virtual ~ClassExtNode() = default;

    std::string_view className;
    std::unique_ptr<MethodNode> methods;
};

struct ClassNode : public Node {
    ClassNode(size_t index, std::string_view name): Node(NodeType::kClass, index), className(name) {}
    virtual ~ClassNode() = default;

    std::string_view className;
    std::string_view superClassName;
    std::string_view optionalName;

    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<MethodNode> methods;
};

struct ReturnNode : public Node {
    ReturnNode(size_t index): Node(NodeType::kReturn, index) {}
    virtual ~ReturnNode() = default;

    std::unique_ptr<Node> valueExpr;
};

/*
struct SlotNode : public Node {
    SlotNode();
    virtual ~SlotNode() = default;
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
*/
struct BlockNode : public Node {
    BlockNode(size_t index): Node(NodeType::kBlock, index) {}
    virtual ~BlockNode() = default;

    std::unique_ptr<VarListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<Node> body;
};

struct ValueNode : public Node {
    ValueNode(size_t index, const TypedValue& v): Node(NodeType::kValue, index), value(v) {}
    virtual ~ValueNode() = default;

    TypedValue value;
};

struct LiteralNode : public Node {
    LiteralNode(size_t index): Node(NodeType::kLiteral, index) {}
    virtual ~LiteralNode() = default;
    // LiteralNodes skip the copy of the TypedValue that resides in the Lexer.
};

struct NameNode : public Node {
    NameNode(size_t index, std::string_view n): Node(NodeType::kName, index), name(n) {}
    virtual ~NameNode() = default;

    std::string_view name;
};

/*
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

struct BlockReturnNode : public Node {
    BlockReturnNode();
    virtual ~BlockReturnNode() = default;
};
*/
} // namespace parse


class Parser {
public:
    Parser(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter);
    ~Parser();

    bool parse();

    const parse::Node* root() const { return m_root.get(); }
    const std::vector<Lexer::Token>& tokens() const { return m_lexer.tokens(); }

private:
    bool next();

    std::unique_ptr<parse::Node> parseRoot();

    std::unique_ptr<parse::ClassNode> parseClass();
    std::unique_ptr<parse::ClassExtNode> parseClassExtension();
    std::unique_ptr<parse::Node> parseCmdLineCode();

    std::unique_ptr<parse::VarListNode> parseClassVarDecls();
    std::unique_ptr<parse::VarListNode> parseClassVarDecl();
    std::unique_ptr<parse::MethodNode> parseMethods();
    std::unique_ptr<parse::MethodNode> parseMethod();
    std::unique_ptr<parse::VarListNode> parseFuncVarDecls();
    std::unique_ptr<parse::VarListNode> parseFuncVarDecl();
    std::unique_ptr<parse::Node> parseFuncBody();

    std::unique_ptr<parse::VarListNode> parseRWVarDefList();
    std::unique_ptr<parse::VarDefNode> parseRWVarDef();
    std::unique_ptr<parse::VarListNode> parseConstDefList();
    std::unique_ptr<parse::VarDefNode> parseConstDef();
    std::unique_ptr<parse::VarListNode> parseVarDefList();
    std::unique_ptr<parse::VarDefNode> parseVarDef();
    std::unique_ptr<parse::ArgListNode> parseArgDecls();

    std::unique_ptr<parse::Node> parseMethodBody();
    std::unique_ptr<parse::Node> parseExprSeq();
    std::unique_ptr<parse::Node> parseExpr();

    Lexer m_lexer;
    size_t m_tokenIndex;
    Lexer::Token m_token;

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<parse::Node> m_root;
};

} // namespace hadron

#endif // SRC_PARSER_HPP_
