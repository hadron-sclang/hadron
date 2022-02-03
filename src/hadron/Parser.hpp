#ifndef SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_

#include "hadron/Hash.hpp"
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
    kEmpty = 0, // Represents a valid parse of an empty input buffer.
    kVarDef = 1,
    kVarList = 2,
    kArgList = 3,
    kMethod = 4,
    kClassExt = 5,
    kClass = 6,
    kReturn = 7,
    kList = 8,
    kDictionary = 9,
    kBlock = 10,
    kLiteral = 11,
    kName = 12,
    kExprSeq = 13,
    kAssign = 14,
    kSetter = 15,
    kKeyValue = 16,
    kCall = 17,
    kBinopCall = 18,
    kPerformList = 19,
    kNumericSeries = 20,
    kCurryArgument = 21,
    kArrayRead = 22,
    kArrayWrite = 23,
    kCopySeries = 24,
    kNew = 25,
    kSeries = 26,
    kSeriesIter = 27,
    kLiteralList = 28,
    kLiteralDict = 29,
    kMultiAssignVars = 30,
    kMultiAssign = 31,
    kIf = 32
};

struct Node {
    Node(NodeType type, size_t index): nodeType(type), tokenIndex(index), tail(this) {}
    Node() = delete;
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

struct ListNode : public Node {
    ListNode(size_t index): Node(NodeType::kList, index) {}
    virtual ~ListNode() = default;

    std::unique_ptr<ExprSeqNode> elements;
};

struct DictionaryNode : public Node {
    DictionaryNode(size_t index): Node(NodeType::kDictionary, index) {}
    virtual ~DictionaryNode() = default;

    std::unique_ptr<ExprSeqNode> elements;
};

struct LiteralNode : public Node {
    LiteralNode(size_t index, Type t, const Slot& v): Node(NodeType::kLiteral, index), type(t), value(v) {}
    virtual ~LiteralNode() = default;

    Type type;
    // Due to unary negation of literals, this value may differ from the token value at tokenIndex. This value should be
    // considered authoritative.
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

    std::unique_ptr<Node> key;
    std::unique_ptr<Node> value;
};

// target.selector(arguments, keyword: arguments)
struct CallNode : public Node {
    CallNode(std::pair<size_t, Hash> sel): Node(NodeType::kCall, sel.first), selector(sel.second) {}
    virtual ~CallNode() = default;

    Hash selector; // TODO: not useful, deprecate
    std::unique_ptr<Node> target;
    std::unique_ptr<Node> arguments;
    std::unique_ptr<KeyValueNode> keywordArguments;
};

struct BinopCallNode : public Node {
    BinopCallNode(size_t index): Node(NodeType::kBinopCall, index) {}
    virtual ~BinopCallNode() = default;

    std::unique_ptr<Node> leftHand;
    std::unique_ptr<Node> rightHand;
    std::unique_ptr<Node> adverb;
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
    std::unique_ptr<KeyValueNode> keywordArguments;
};

struct NumericSeriesNode : public Node {
    NumericSeriesNode(size_t index): Node(NodeType::kNumericSeries, index) {}
    virtual ~NumericSeriesNode() = default;

    std::unique_ptr<Node> start;
    std::unique_ptr<Node> step;
    std::unique_ptr<Node> stop;
};

struct CurryArgumentNode : public Node {
    CurryArgumentNode(size_t index): Node(NodeType::kCurryArgument, index) {}
    virtual ~CurryArgumentNode() = default;
};

// Normally translated to the "at" method, but parsed as a distinct parse node to allow for possible compiler inlining
// of array access.
struct ArrayReadNode : public Node {
    ArrayReadNode(size_t index): Node(NodeType::kArrayRead, index) {}
    virtual ~ArrayReadNode() = default;

    std::unique_ptr<Node> targetArray;
    std::unique_ptr<ExprSeqNode> indexArgument;
};

struct ArrayWriteNode : public Node {
    ArrayWriteNode(size_t index): Node(NodeType::kArrayWrite, index) {}
    virtual ~ArrayWriteNode() = default;

    // targetArray[indexArgument] = value
    std::unique_ptr<Node> targetArray;
    std::unique_ptr<ExprSeqNode> indexArgument;
    std::unique_ptr<Node> value;
};

// Syntax shorthand for subarray copies, e.g. target[1,2..4]
struct CopySeriesNode : public Node {
    CopySeriesNode(size_t index): Node(NodeType::kCopySeries, index) {}
    virtual ~CopySeriesNode() = default;

    std::unique_ptr<Node> target;
    std::unique_ptr<ExprSeqNode> first;
    std::unique_ptr<ExprSeqNode> last;
};

// Syntax shorthand for a call to the new() method.
struct NewNode : public Node {
    NewNode(size_t index): Node(NodeType::kNew, index) {}
    virtual ~NewNode() = default;

    std::unique_ptr<Node> target;
    std::unique_ptr<Node> arguments;
    std::unique_ptr<KeyValueNode> keywordArguments;
};

// Equivalent to start.series(step, last)
struct SeriesNode : public Node {
    SeriesNode(size_t index): Node(NodeType::kSeries, index) {}
    virtual ~SeriesNode() = default;

    std::unique_ptr<ExprSeqNode> start;
    std::unique_ptr<ExprSeqNode> step;
    std::unique_ptr<ExprSeqNode> last;
};

struct SeriesIterNode : public Node {
    SeriesIterNode(size_t index): Node(NodeType::kSeriesIter, index) {}
    virtual ~SeriesIterNode() = default;

    std::unique_ptr<ExprSeqNode> start;
    std::unique_ptr<ExprSeqNode> step;
    std::unique_ptr<ExprSeqNode> last;
};

struct LiteralListNode : public Node {
    LiteralListNode(size_t index): Node(NodeType::kLiteralList, index) {}
    virtual ~LiteralListNode() = default;

    std::unique_ptr<NameNode> className;
    std::unique_ptr<Node> elements;
};

struct LiteralDictNode : public Node {
    LiteralDictNode(size_t index): Node(NodeType::kLiteralDict, index) {}
    virtual ~LiteralDictNode() = default;

    std::unique_ptr<Node> elements;
};

struct MultiAssignVarsNode : public Node {
    MultiAssignVarsNode(size_t index): Node(NodeType::kMultiAssignVars, index) {}
    virtual ~MultiAssignVarsNode() = default;

    std::unique_ptr<NameNode> names;
    std::unique_ptr<NameNode> rest;
};

struct MultiAssignNode : public Node {
    MultiAssignNode(size_t index): Node(NodeType::kMultiAssign, index) {}
    virtual ~MultiAssignNode() = default;

    std::unique_ptr<MultiAssignVarsNode> targets;
    std::unique_ptr<Node> value;
};

struct IfNode : public Node {
    IfNode(size_t index): Node(NodeType::kIf, index) {}
    virtual ~IfNode() = default;

    std::unique_ptr<ExprSeqNode> condition;
    std::unique_ptr<BlockNode> trueBlock;
    // optional else condition.
    std::unique_ptr<BlockNode> falseBlock;
};

} // namespace parse

class Parser {
public:
    // Builds a parse tree from an external lexer that has already successfully lexed the source code.
    Parser(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter);

    // Used for testing, lexes the code itself with an owned Lexer first.
    Parser(std::string_view code);
    ~Parser();

    // Parse interpreter input. root() will always be a BlockNode or an empty Node (in event of empty input)
    bool parse();
    // Parse input with class definitions or class extensions. root() will be a ClassNode or ClassExtNode.
    bool parseClass();

    const parse::Node* root() const { return m_root.get(); }
    Lexer* lexer() const { return m_lexer; }
    std::shared_ptr<ErrorReporter> errorReporter() { return m_errorReporter; }

    // Access to parser from Bison Parser
    void addRoot(std::unique_ptr<parse::Node> root);
    Token token(size_t index) const;
    size_t tokenIndex() const { return m_tokenIndex; }
    void next() { ++m_tokenIndex; }
    bool sendInterpret() const { return m_sendInterpret; }
    void setInterpret(bool i) { m_sendInterpret = i; }

private:
    bool innerParse();

    std::unique_ptr<Lexer> m_ownLexer;
    Lexer* m_lexer;
    size_t m_tokenIndex;
    bool m_sendInterpret;

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<parse::Node> m_root;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_
