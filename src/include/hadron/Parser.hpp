#ifndef SRC_INCLUDE_HADRON_PARSER_HPP_
#define SRC_INCLUDE_HADRON_PARSER_HPP_

#include "hadron/Slot.hpp"
#include "hadron/Token.hpp"
#include "hadron/Type.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace hadron {

class ErrorReporter;
class Lexer;

namespace parse {

enum NodeType {
    kEmpty = 0,
    kVarDef = 1,
    kVarList = 2,
    kArgList = 3,
    kMethod = 4,
    kClassExt = 5,
    kClass = 6,
    kReturn = 7,
    kDynList = 8,
    kBlock = 9,
    kLiteral = 10,
    kName = 11,
    kExprSeq = 12,
    kAssign = 13,
    kSetter = 14,
    kKeyValue = 15,
    kCall = 16,
    kBinopCall = 17,
    kPerformList = 18,
    kNumericSeries = 19
};

struct Node {
    Node() = delete;
    explicit Node(NodeType type, size_t index): nodeType(type), tokenIndex(index), tail(this) {}
    virtual ~Node() = default;
    void append(std::unique_ptr<Node> node) {
        tail->next = std::move(node);
        tail = tail->next->tail;
    }

    NodeType nodeType;
    size_t tokenIndex;
    std::unique_ptr<Node> next;
    Node* tail;
};

struct VarDefNode : public Node {
    VarDefNode(size_t index): Node(NodeType::kVarDef, index) {}
    virtual ~VarDefNode() = default;
    bool hasReadAccessor = false;
    bool hasWriteAccessor = false;

    std::unique_ptr<Node> initialValue;
};

struct VarListNode : public Node {
    VarListNode(size_t index): Node(NodeType::kVarList, index) {}
    virtual ~VarListNode() = default;

    // The associated Lexer Token can be used to disambiguate between classvar, var, and const declarations.
    std::unique_ptr<VarDefNode> definitions;
};

struct ArgListNode : public Node {
    ArgListNode(size_t index): Node(NodeType::kArgList, index) {}
    virtual ~ArgListNode() = default;

    std::unique_ptr<VarListNode> varList;
    std::optional<size_t> varArgsNameIndex;
};

struct ExprSeqNode : public Node {
    ExprSeqNode(size_t index, std::unique_ptr<Node> firstExpr):
        Node(NodeType::kExprSeq, index), expr(std::move(firstExpr)) {}
    virtual ~ExprSeqNode() = default;

    std::unique_ptr<Node> expr;
};

struct BlockNode : public Node {
    BlockNode(size_t index): Node(NodeType::kBlock, index) {}
    virtual ~BlockNode() = default;

    std::unique_ptr<ArgListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<ExprSeqNode> body;
};

struct MethodNode : public Node {
    MethodNode(size_t index, bool classMethod):
            Node(NodeType::kMethod, index),
            isClassMethod(classMethod) {}
    virtual ~MethodNode() = default;

    bool isClassMethod;
    std::optional<size_t> primitiveIndex;
    std::unique_ptr<BlockNode> body;
};

struct ClassExtNode : public Node {
    ClassExtNode(size_t index): Node(NodeType::kClassExt, index) {}
    virtual ~ClassExtNode() = default;

    std::unique_ptr<MethodNode> methods;
};

struct ClassNode : public Node {
    ClassNode(size_t index): Node(NodeType::kClass, index) {}
    virtual ~ClassNode() = default;

    std::optional<size_t> superClassNameIndex;
    std::optional<size_t> optionalNameIndex;

    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<MethodNode> methods;
};

struct ReturnNode : public Node {
    ReturnNode(size_t index): Node(NodeType::kReturn, index) {}
    virtual ~ReturnNode() = default;

    // nullptr means default return value.
    std::unique_ptr<Node> valueExpr;
};

struct DynListNode : public Node {
    DynListNode(size_t index): Node(NodeType::kDynList, index) {}
    virtual ~DynListNode() = default;

    std::unique_ptr<Node> elements;
};

struct LiteralNode : public Node {
    LiteralNode(size_t index, const Slot& v): Node(NodeType::kLiteral, index), value(v) {}
    virtual ~LiteralNode() = default;

    // Due to unary negation of literals, this value may differ from the token value at tokenIndex.
    Slot value;
    // If blockLiteral is non-null, this is a blockLiteral and the Slot is ignored.
    std::unique_ptr<BlockNode> blockLiteral;
};

struct NameNode : public Node {
    NameNode(size_t index): Node(NodeType::kName, index) {}
    virtual ~NameNode() = default;

    bool isGlobal = false;
};

struct KeyValueNode : public Node {
    KeyValueNode(size_t index): Node(NodeType::kKeyValue, index) {}
    virtual ~KeyValueNode() = default;

    std::unique_ptr<Node> value;
};

// target.selector(arguments, keyword: arguments)
// target can also be null, in which case target is assumed to be the first argument, for example:
// while({x < 5}, { /* code */ });
// blocklists are appended to arguments, so this syntax will result in the same construction:
// while { x < 5 } { /* code */ };
struct CallNode : public Node {
    CallNode(size_t index): Node(NodeType::kCall, index) {}
    virtual ~CallNode() = default;

    std::unique_ptr<Node> target;
    std::unique_ptr<Node> arguments;
    std::unique_ptr<KeyValueNode> keywordArguments;
};

struct BinopCallNode : public Node {
    BinopCallNode(size_t index): Node(NodeType::kBinopCall, index) {}
    virtual ~BinopCallNode() = default;

    std::unique_ptr<Node> leftHand;
    std::unique_ptr<Node> rightHand;
};

// From an = command, assigns value to the identifier in name.
struct AssignNode : public Node {
    AssignNode(size_t index): Node(NodeType::kAssign, index) {}
    virtual ~AssignNode() = default;

    std::unique_ptr<NameNode> name;
    std::unique_ptr<Node> value;
};

// target.selector = value, token should point at selector
struct SetterNode : public Node {
    SetterNode(size_t index): Node(NodeType::kSetter, index) {}
    virtual ~SetterNode() = default;

    // The recipient of the assigned value.
    std::unique_ptr<Node> target;
    std::unique_ptr<Node> value;
};

// Below nodes are higher-level syntax constructs that LSC processes into lower-level function calls during parsing.
// We keep these high-level for first parsing pass.
struct PerformListNode : public Node {
    PerformListNode(size_t index): Node(NodeType::kPerformList, index) {}
    virtual ~PerformListNode() = default;

    std::unique_ptr<Node> target;
    std::unique_ptr<Node> arguments;
};

struct NumericSeriesNode : public Node {
    NumericSeriesNode(size_t index): Node(NodeType::kNumericSeries, index) {}
    virtual ~NumericSeriesNode() = default;

    std::unique_ptr<Node> start;
    std::unique_ptr<Node> step;
    std::unique_ptr<Node> stop;
};

} // namespace parse


class Parser {
public:
    // Builds a parse tree from an external lexer that has already successfully lexed the source code.
    Parser(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter);

    // Used for testing, lexes the code itself with an owned Lexer first.
    Parser(std::string_view code);
    ~Parser();

    bool parse();

    const parse::Node* root() const { return m_root.get(); }
    Lexer* lexer() const { return m_lexer; }
    std::shared_ptr<ErrorReporter> errorReporter() { return m_errorReporter; }

    // Access to parser from Bison Parser
    void setRoot(std::unique_ptr<parse::Node> root);
    Token token(size_t index) const;
    size_t tokenIndex() const { return m_tokenIndex; }
    bool next();

private:

    std::unique_ptr<Lexer> m_ownLexer;
    Lexer* m_lexer;
    size_t m_tokenIndex;
    Token m_token;

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<parse::Node> m_root;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_PARSER_HPP_
