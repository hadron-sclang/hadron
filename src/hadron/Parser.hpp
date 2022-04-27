#ifndef SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_PARSER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"
#include "hadron/Token.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace hadron {

class ErrorReporter;
class Lexer;

namespace parse {

enum NodeType {
    kArgList,
    kArray,
    kArrayRead,
    kArrayWrite,
    kAssign,
    kBinopCall,
    kBlock,
    kCall,
    kClass,
    kClassExt,
    kCopySeries,
    kCurryArgument,
    kEmpty,
    kEnvironmentAt,
    kEnvironmentPut,
    kEvent,
    kExprSeq,
    kIf,
    kKeyValue,
    kLiteralDict, // kNewEvent - these are calls and allow curry args, created at runtime
    kLiteralList, // kNewCollection
    kMethod,
    kMultiAssign,
    kMultiAssignVars,
    kName,
    kNew,
    kNumericSeries,
    kPerformList,
    kReturn,
    kSeries,
    kSeriesIter,
    kSetter,
    kSlot,
    kString,
    kSymbol,
    kValue,
    kVarDef,
    kVarList,
    kWhile
};

struct Node {
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

protected:
    Node(NodeType type, size_t index): nodeType(type), tokenIndex(index), tail(this) {}
};

struct KeyValueNode;

// Several Call-type parse nodes have the same member variables so we aggregate them in a base class.
struct CallBaseNode : public Node {
    CallBaseNode() = delete;
    virtual ~CallBaseNode() = default;

    std::unique_ptr<Node> target;
    std::unique_ptr<Node> arguments;
    std::unique_ptr<KeyValueNode> keywordArguments;

protected:
    CallBaseNode(NodeType type, size_t index): Node(type, index) {}
};

struct VarListNode;

struct ArgListNode : public Node {
    ArgListNode(size_t index): Node(NodeType::kArgList, index) {}
    virtual ~ArgListNode() = default;

    std::unique_ptr<VarListNode> varList;
    std::optional<size_t> varArgsNameIndex;
};

struct ExprSeqNode;

// An array of elements without classname, e.g. [1, 2, 3], always makes an Array
struct ArrayNode : public Node {
    ArrayNode(size_t index): Node(NodeType::kArray, index) {}
    virtual ~ArrayNode() = default;

    std::unique_ptr<ExprSeqNode> elements;
};

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

struct NameNode;

// From an = command, assigns value to the identifier in name.
struct AssignNode : public Node {
    AssignNode(size_t index): Node(NodeType::kAssign, index) {}
    virtual ~AssignNode() = default;

    std::unique_ptr<NameNode> name;
    std::unique_ptr<Node> value;
};

struct BinopCallNode : public Node {
    BinopCallNode(size_t index): Node(NodeType::kBinopCall, index) {}
    virtual ~BinopCallNode() = default;

    std::unique_ptr<Node> leftHand;
    std::unique_ptr<Node> rightHand;
    std::unique_ptr<Node> adverb;
};

struct BlockNode : public Node {
    BlockNode(size_t index): Node(NodeType::kBlock, index) {}
    virtual ~BlockNode() = default;

    // Destructively move everything in this block to a freshly allocated object.
    std::unique_ptr<BlockNode> moveTo() {
        auto block = std::make_unique<BlockNode>(tokenIndex);
        block->next = std::move(next);
        next = nullptr;
        block->tail = tail;
        tail = nullptr;
        block->arguments = std::move(arguments);
        arguments = nullptr;
        block->variables = std::move(variables);
        variables = nullptr;
        block->body = std::move(body);
        body = nullptr;
        return block;
    }

    std::unique_ptr<ArgListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<ExprSeqNode> body;
};

// target.selector(arguments, keyword: arguments)
struct CallNode : public CallBaseNode {
    CallNode(size_t index): CallBaseNode(NodeType::kCall, index) {}
    virtual ~CallNode() = default;
};

struct MethodNode;

struct ClassNode : public Node {
    ClassNode(size_t index): Node(NodeType::kClass, index) {}
    virtual ~ClassNode() = default;

    std::optional<size_t> superClassNameIndex;
    std::optional<size_t> optionalNameIndex;

    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<MethodNode> methods;
};

struct ClassExtNode : public Node {
    ClassExtNode(size_t index): Node(NodeType::kClassExt, index) {}
    virtual ~ClassExtNode() = default;

    std::unique_ptr<MethodNode> methods;
};

// Syntax shorthand for subarray copies, e.g. target[1,2..4]
struct CopySeriesNode : public Node {
    CopySeriesNode(size_t index): Node(NodeType::kCopySeries, index) {}
    virtual ~CopySeriesNode() = default;

    std::unique_ptr<Node> target;
    std::unique_ptr<ExprSeqNode> first;
    std::unique_ptr<Node> second;
    std::unique_ptr<ExprSeqNode> last;
};

struct CurryArgumentNode : public Node {
    CurryArgumentNode(size_t index): Node(NodeType::kCurryArgument, index) {}
    virtual ~CurryArgumentNode() = default;
};

struct EmptyNode : public Node {
    EmptyNode(): Node(NodeType::kEmpty, 0) {}
    virtual ~EmptyNode() = default;
};

struct EnvironmentAtNode : public Node {
    EnvironmentAtNode(size_t index): Node(NodeType::kEnvironmentAt, index) {}
    virtual ~EnvironmentAtNode() = default;
};

struct EnvironmentPutNode : public Node {
    EnvironmentPutNode(size_t index): Node(NodeType::kEnvironmentPut, index) {}
    virtual ~EnvironmentPutNode() = default;

    std::unique_ptr<Node> value;
};

  // A keyword/value pair in parens makes an event e.g. (a: 4, b: 5) always makes an Event
struct EventNode : public Node {
    EventNode(size_t index): Node(NodeType::kEvent, index) {}
    virtual ~EventNode() = default;

    // Expected to be in pairs of key: value.
    std::unique_ptr<ExprSeqNode> elements;
};

struct ExprSeqNode : public Node {
    ExprSeqNode(size_t index, std::unique_ptr<Node> firstExpr):
        Node(NodeType::kExprSeq, index), expr(std::move(firstExpr)) {}
    virtual ~ExprSeqNode() = default;

    std::unique_ptr<Node> expr;
};

struct IfNode : public Node {
    IfNode(size_t index): Node(NodeType::kIf, index) {}
    virtual ~IfNode() = default;

    std::unique_ptr<ExprSeqNode> condition;
    std::unique_ptr<BlockNode> trueBlock;
    // optional else condition.
    std::unique_ptr<BlockNode> falseBlock;
};

struct KeyValueNode : public Node {
    KeyValueNode(size_t index): Node(NodeType::kKeyValue, index) {}
    virtual ~KeyValueNode() = default;

    std::unique_ptr<Node> key;
    std::unique_ptr<Node> value;
};

struct LiteralDictNode : public Node {
    LiteralDictNode(size_t index): Node(NodeType::kLiteralDict, index) {}
    virtual ~LiteralDictNode() = default;

    std::unique_ptr<Node> elements;
};

struct LiteralListNode : public Node {
    LiteralListNode(size_t index): Node(NodeType::kLiteralList, index) {}
    virtual ~LiteralListNode() = default;

    std::unique_ptr<NameNode> className;
    std::unique_ptr<Node> elements;
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

struct MultiAssignVarsNode;

struct MultiAssignNode : public Node {
    MultiAssignNode(size_t index): Node(NodeType::kMultiAssign, index) {}
    virtual ~MultiAssignNode() = default;

    std::unique_ptr<MultiAssignVarsNode> targets;
    std::unique_ptr<Node> value;
};

struct MultiAssignVarsNode : public Node {
    MultiAssignVarsNode(size_t index): Node(NodeType::kMultiAssignVars, index) {}
    virtual ~MultiAssignVarsNode() = default;

    std::unique_ptr<NameNode> names;
    std::unique_ptr<NameNode> rest;
};

struct NameNode : public Node {
    NameNode(size_t index): Node(NodeType::kName, index) {}
    virtual ~NameNode() = default;
};

// Syntax shorthand for a call to the new() method.
struct NewNode : public CallBaseNode {
    NewNode(size_t index): CallBaseNode(NodeType::kNew, index) {}
    virtual ~NewNode() = default;
};

struct NumericSeriesNode : public Node {
    NumericSeriesNode(size_t index): Node(NodeType::kNumericSeries, index) {}
    virtual ~NumericSeriesNode() = default;

    std::unique_ptr<Node> start;
    std::unique_ptr<Node> step;
    std::unique_ptr<Node> stop;
};

struct PerformListNode : public CallBaseNode {
    PerformListNode(size_t index): CallBaseNode(NodeType::kPerformList, index) {}
    virtual ~PerformListNode() = default;
};

struct ReturnNode : public Node {
    ReturnNode(size_t index): Node(NodeType::kReturn, index) {}
    virtual ~ReturnNode() = default;

    // nullptr means default return value.
    std::unique_ptr<Node> valueExpr;
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

// target.selector = value, token should point at selector
struct SetterNode : public Node {
    SetterNode(size_t index): Node(NodeType::kSetter, index) {}
    virtual ~SetterNode() = default;

    // The recipient of the assigned value.
    std::unique_ptr<Node> target;
    std::unique_ptr<Node> value;
};

// Holds any literal that can fit in a Slot without memory allocation, so int32, double, bool, char, nil.
struct SlotNode : public Node {
    SlotNode(size_t index, const Slot& v): Node(NodeType::kSlot, index), value(v) {}
    virtual ~SlotNode() = default;

    // Due to unary negation of literals, this value may differ from the token value at tokenIndex. This value should be
    // considered authoritative.
    Slot value;
};

// StringNode->next may point at additional StringNodes that should be concatenated to this one when lowering to AST.
struct StringNode : public Node {
    StringNode(size_t index): Node(NodeType::kString, index) {}
    virtual ~StringNode() = default;
};

// References a literal Symbol in the source code.
struct SymbolNode : public Node {
    SymbolNode(size_t index): Node(NodeType::kSymbol, index) {}
    virtual ~SymbolNode() = default;
};

// Implied evaluation of a function, an implied call to value, like "f.(a, b)"
struct ValueNode : public CallBaseNode {
    explicit ValueNode(size_t index): CallBaseNode(NodeType::kValue, index) {}
    virtual ~ValueNode() = default;
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

struct WhileNode : public Node {
    WhileNode(size_t index): Node(NodeType::kWhile, index) {}
    virtual ~WhileNode() = default;

    // First block is the condition block, subsequent blocks are optional.
    std::unique_ptr<BlockNode> blocks;
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
