#ifndef SRC_PARSER_HPP_
#define SRC_PARSER_HPP_

#include "Lexer.hpp"

#include <memory>
#include <string_view>

namespace hadron {

class ErrorReporter;

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

struct LiteralNode : public Node {
    LiteralNode();
    virtual ~LiteralNode() = default;
};

struct VarDefNode : public Node {
    VarDefNode(std::string_view name): varName(name) {}
    virtual ~VarDefNode() = default;

    std::string_view varName;
    std::unique_ptr<Node> initialValue;
};

struct VarListNode : public Node {
    VarListNode();
    virtual ~VarListNode() = default;

    std::unique_ptr<VarDefNode> definitions;
};

struct ArgListNode : public Node {
    ArgListNode();
    virtual ~ArgListNode() = default;

    std::unique_ptr<VarListNode> varList;
    std::string_view varArgsName;
};

struct MethodNode : public Node {
    MethodNode(std::string_view name, bool classMethod): methodName(name), isClassMethod(classMethod) {}
    virtual ~MethodNode() = default;

    std::string_view methodName;
    bool isClassMethod;

    std::unique_ptr<ArgListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<Node> body;
};

struct ClassExtNode : public Node {
    ClassExtNode(std::string_view name): className(name) {}
    virtual ~ClassExtNode() = default;

    std::string_view className;
    std::unique_ptr<MethodNode> methods;
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

struct ReturnNode : public Node {
    ReturnNode();
    virtual ~ReturnNode() = default;

    std::unique_ptr<Node> valueExpr;
};


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

struct BlockNode : public Node {
    BlockNode() = default;
    virtual ~BlockNode() = default;

    std::unique_ptr<VarListNode> arguments;
    std::unique_ptr<VarListNode> variables;
    std::unique_ptr<Node> body;
};

// TODO: reconsider name to something like ValueNode - holds a flexibly-typed value.

// LSC typedefs this and SlotDef to a base Slot type, relying on the expr1 grammar to prevent nonsensical expressions
// like "a" = "not A". One can imagine a variety of optimizations that might depend on knowing this is a literal value
// instead of a variable slot, also type inference (which is definitely optimization but may not be exclusively
// optimization). So if something like var a = 5; well then a's type is obviously an integer. So there's an optimization
// that could then be done to determine if the type of a is clearly an integer for its entire lifetime, and, if so, then
// we can emit JIT bytecode that treats a as a native int. But the general case is always going to be to treat
// everything as a slot with messaging. Worth studying how V8 does some optimizations to optimize certain code paths by
// collecting runtime statistics on type of variables, then emitting optimized bytecode for certain types, but
// definitely for another day.
struct SlotDefNode : public Node {
    SlotDefNode();
    virtual ~SlotDefNode() = default;
};
struct PushLiteralNode : public Node {
    PushLiteralNode();
    virtual ~PushLiteralNode() = default;
};

struct ValueNode : public Node {
    enum Type {
        kClass,  // The (singleton) Class object
        kObject  // An *instance* of a Class
    };
    ValueNode(Type t): type(t) {}
    virtual ~ValueNode() = default;

    Type type;

    union Value {
        void* classPointer;
        void* objectPointer;
    };
    Value value;
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

struct BlockReturnNode : public Node {
    BlockReturnNode();
    virtual ~BlockReturnNode() = default;
};

} // namespace parse


class Parser {
public:
    Parser(std::string_view code, std::shared_ptr<ErrorReporter> errorReporter);
    ~Parser();

    bool parse();

private:
    bool next();

    std::unique_ptr<parse::Node> parseRoot();

    std::unique_ptr<parse::ClassNode> parseClass();
    std::unique_ptr<parse::ClassExtNode> parseClassExtension();
    std::unique_ptr<parse::Node> parseCmdLineCode();

    std::unique_ptr<parse::VarListNode> parseClassVarDecls();
    std::unique_ptr<parse::VarListNode> parseClassVarDecl();
    std::unique_ptr<parse::MethodNode> parseMethods();
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

    std::unique_ptr<parse::LiteralNode> parseLiteral();

    Lexer m_lexer;
    size_t m_tokenIndex;
    Lexer::Token m_token;

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<parse::Node> m_root;
};

} // namespace hadron

#endif // SRC_PARSER_HPP_
