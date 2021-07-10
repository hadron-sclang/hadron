#ifndef SRC_PARSER_HPP_
#define SRC_PARSER_HPP_

#include "hadron/Lexer.hpp"
#include "hadron/Literal.hpp"
#include "hadron/Type.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace hadron {

class ErrorReporter;

namespace parse {

enum NodeType {
    kEmpty,
    kVarDef,
    kVarList,
    kArgList,
    kMethod,
    kClassExt,
    kClass,
    kReturn,
    kDynList,
    kBlock,
    kValue,
    kLiteral,
    kName,
    kExprSeq,
    kAssign,
    kSetter,
    kKeyValue,
    kCall,
    kBinopCall,
    kPerformList,
    kNumericSeries
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

struct BlockNode : public Node {
    BlockNode(size_t index): Node(NodeType::kBlock, index) {}
    virtual ~BlockNode() = default;

    std::unique_ptr<ArgListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<Node> body;
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
*/

struct DynListNode : public Node {
    DynListNode(size_t index): Node(NodeType::kDynList, index) {}
    virtual ~DynListNode() = default;

    std::unique_ptr<Node> elements;
};

/*
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

struct LiteralNode : public Node {
    LiteralNode(size_t index, const Literal& v): Node(NodeType::kLiteral, index), value(v) {}
    virtual ~LiteralNode() = default;

    // Due to unary negation of literals, this value may differ from the token value at tokenIndex.
    Literal value;
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

// DropNodes in LSC represent exprs that could be potentially dropped, such that
// "1; 2; 3;" would result in expr "1" and "2" being dropped and never being evaluated.
// This is a parse tree transformation that we would prefer to do as a separate step.
// The different optimization steps will be easier to design, configure, and verify if
// they happen in discrete steps, rather than all at once.
/*
struct DropNode : public Node {
    DropNode();
    virtual ~DropNode() = default;
};
*/

struct ExprSeqNode : public Node {
    ExprSeqNode(size_t index, std::unique_ptr<Node> firstExpr):
        Node(NodeType::kExprSeq, index), expr(std::move(firstExpr)) {}
    virtual ~ExprSeqNode() = default;

    std::unique_ptr<Node> expr;
};

// From an = command, assigns value to the identifier in name.
struct AssignNode : public Node {
    AssignNode(size_t index): Node(NodeType::kAssign, index) {}
    virtual ~AssignNode() = default;

    std::unique_ptr<NameNode> name;
    std::unique_ptr<Node> value;
};

/*
struct MultiAssignNode : public Node {
    MultiAssignNode();
    virtual ~MultiAssignNode() = default;
};
*/

// target.selector = value
struct SetterNode : public Node {
    SetterNode(size_t index): Node(NodeType::kSetter, index) {}
    virtual ~SetterNode() = default;

    // The recipient of the assigned value.
    std::unique_ptr<Node> target;
    std::unique_ptr<Node> value;
};

/*
struct CurryArgNode : public Node {
    CurryArgNode();
    virtual ~CurryArgNode() = default;
};

struct BlockReturnNode : public Node {
    BlockReturnNode();
    virtual ~BlockReturnNode() = default;
};
*/

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
    Parser(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter);
    // Used for testing, makes a parser with suppressed error messages.
    Parser(std::string_view code);
    ~Parser();

    bool parse();

    const parse::Node* root() const { return m_root.get(); }
    const std::vector<Lexer::Token>& tokens() const { return m_lexer.tokens(); }
    std::shared_ptr<ErrorReporter> errorReporter() { return m_errorReporter; }

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
    std::unique_ptr<parse::VarListNode> parseSlotDefList();
    std::unique_ptr<parse::VarDefNode> parseSlotDef();
    std::unique_ptr<parse::ArgListNode> parseArgDecls();

    std::unique_ptr<parse::Node> parseMethodBody();
    std::unique_ptr<parse::Node> parseExprSeq();
    std::unique_ptr<parse::Node> parseExpr(bool captureSuffixes = true);

    // If the current token is a literal, or a unary negation token followed by an int or float literal, consumes those
    // tokens and returns a LiteralNode with the value. Otherwise consumes no tokens and returns nullptr.
    std::unique_ptr<parse::LiteralNode> parseLiteral();

    std::unique_ptr<parse::Node> parseArrayElements();
    std::unique_ptr<parse::Node> parseArgList();
    std::unique_ptr<parse::KeyValueNode> parseKeywordArgList();
    std::unique_ptr<parse::Node> parseBlockList();
    std::unique_ptr<parse::Node> parseBlockOrGenerator();

    Lexer m_lexer;
    size_t m_tokenIndex;
    Lexer::Token m_token;

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<parse::Node> m_root;
};

} // namespace hadron

#endif // SRC_PARSER_HPP_
