#ifndef SRC_HADRON_LIBRARY_HADRON_PARSE_NODE_HPP_
#define SRC_HADRON_LIBRARY_HADRON_PARSE_NODE_HPP_

#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronParseNodeSchema.hpp"

namespace hadron {
namespace library {

class Token : public Object<Token, schema::HadronLexTokenSchema> {
public:
    Token(): Object<Token, schema::HadronLexTokenSchema>() {}
    explicit Token(schema::HadronLexTokenSchema* instance): Object<Token, schema::HadronLexTokenSchema>(instance) {}
    explicit Token(Slot instance): Object<Token, schema::HadronLexTokenSchema>(instance) {}
    ~Token() {}

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol s) { m_instance->name = s.slot(); }

    Slot value() const { return m_instance->value; }
    void setValue(Slot s) { m_instance->value = s; }

    int32_t lineNumber() const { return m_instance->lineNumber.getInt32(); }
    void setLineNumber(int32_t i) { m_instance->lineNumber = Slot::makeInt32(i); }

    int32_t characterNumber() const { return m_instance->characterNumber.getInt32(); }
    void setCharacterNumber(int32_t i) { m_instance->characterNumber = Slot::makeInt32(i); }

    int32_t offset() const { return m_instance->offset.getInt32(); }
    void setOffset(int32_t i) { m_instance->offset = Slot::makeInt32(i); }

    int32_t length() const { return m_instance->length.getInt32(); }
    void setLength(int32_t i) { m_instance->length = Slot::makeInt32(i); }
};

template<typename T, typename S, typename B>
class NodeBase : public Object<T, S> {
public:
    NodeBase(): Object<T, S>() {}
    explicit NodeBase(S* instance): Object<T, S>(instance) {}
    explicit NodeBase(Slot instance): Object<T, S>(instance) {}
    ~NodeBase() {}

    B toBase() const {
        const T& t = static_cast<const T&>(*this);
        return B::wrapUnsafe(Slot::makePointer(reinterpret_cast<library::Schema*>(t.m_instance)));
    }

    Token token() const {
        const T& t = static_cast<const T&>(*this);
        return Token(t.m_instance->token.slot());
    }
    void setToken(Token tok) {
        T& t = static_cast<T&>(*this);
        t.m_instance->token = tok.slot();
    }

    B next() const {
        const T& t = static_cast<const T&>(*this);
        return B::wrapUnsafe(t.m_instance->next);
    }
    void setNext(B b) {
        T& t = static_cast<T&>(*this);
        t.m_instance->next = b.slot();
    }
};

class Node : public NodeBase<Node, schema::HadronParseNodeSchema, Node> {
public:
    Node(): NodeBase<Node, schema::HadronParseNodeSchema, Node>() {}
    explicit Node(schema::HadronParseNodeSchema* instance):
            NodeBase<Node, schema::HadronParseNodeSchema, Node>(instance) {}
    explicit Node(Slot instance): NodeBase<Node, schema::HadronParseNodeSchema, Node>(instance) {}
    ~Node() {}
};

class KeyValueNode : public NodeBase<KeyValueNode, schema::HadronKeyValueNodeSchema, Node> {
public:
    KeyValueNode(): NodeBase<KeyValueNode, schema::HadronKeyValueNodeSchema, Node>() {}
    explicit KeyValueNode(schema::HadronKeyValueNodeSchema* instance):
            NodeBase<Node, schema::HadronKeyValueNodeSchema, Node>(instance) {}
    explicit KeyValueNode(Slot instance):
            NodeBase<Node, schema::HadronKeyValueNodeSchema, Node>(instance) {}
    ~KeyValueNode() {}

    Node key() const { return Node::wrapUnsafe(m_instance->key); }
    void setKey(Node n) { m_instance->key = n.slot(); }

    void value() const { return Node::wrapUnsafe(m_instance->value); }
    void setValue(Node n) { m_instance->value = n.slot(); }
};

template<typename T, typename S>
class CallBaseNode : public NodeBase<T, S, Node> {
public:
    CallBaseNode(): NodeBase<T, S, Node>() {}
    explicit CallBaseNode(S* instance): NodeBase<T, S, Node>(instance) {}
    explicit CallBaseNode(Slot instance): NodeBase<T, S, Node>(instance) {}
    ~CallBaseNode() {}

    Node target() const {
        const T& t = static_cast<const T&>(*this);
        return Node::wrapUnsafe(t.m_instance->target);
    }
    void setTarget(Node n) {
        T& t = static_cast<T&>(*this);
        t.m_instance->target = n.slot();
    }

    Node arguments() const {
        const T& t = static_cast<const T&>(*this);
        return Node::wrapUnsafe(t.m_instance->arguments);
    }
    void setArguments(Node n) {
        T& t = static_cast<T&>(*this);
        t.m_instance->arguments = n.slot();
    }

    KeyValueNode keywordArguments() const {
        const T& t = static_cast<const T&>(*this);
        return KeyValueNode(t.m_instance->keywordArguments);
    }
    void setKeywordArguments(KeyValueNode n) {
        T& t = static_cast<T&>(*this);
        t.m_instance->keywordArguemnts = n.slot();
     }
};

class VarDefNode : public NodeBase<VarDefNode, schema::HadronVarDefNodeSchema, Node> {
public:
    VarDefNode(): NodeBase<VarDefNode, schema::HadronVarDefNodeSchema, Node>() {}
    explicit VarDefNode(schema::HadronVarDefNodeSchema* instance):
            NodeBase<VarDefNode, schema::HadronVarDefNodeSchema, Node>(instance) {}
    explicit VarDefNode(Slot instance):
            NodeBase<VarDefNode, schema::HadronVarDefNodeSchema, Node>(instance) {}
    ~VarDefNode() {}

    Node initialValue() const { return Node::wrapUnsafe(m_instance->initialValue); }
    void setInitialValue(Node n) { m_instance->initialValue = n.slot(); }
};

class VarListNode : public NodeBase<VarListNode, schema::HadronVarListNodeSchema, Node> {
public:
    VarListNode(): NodeBase<VarListNode, schema::HadronVarListNodeSchema, Node>() {}
    explicit VarListNode(schema::HadronVarListNodeSchema* instance):
            NodeBase<VarListNode, schema::HadronVarListNodeSchema, Node>(instance) {}
    explicit VarListNode(Slot instance):
            NodeBase<VarListNode, schema::HadronVarListNodeSchema, Node>(instance) {}
    ~VarListNode() {}

    VarDefNode definitions() const { return VarDefNode(m_instance->definitions); }
    void setDefinitions(VarDefNode varDef) { m_instance->definitions = varDef.slot(); }
};

class ArgListNode : public NodeBase<ArgListNode, schema::HadronArgListNodeSchema, Node> {
public:
    ArgListNode(): NodeBase<ArgListNode, schema::HadronArgListNodeSchema, Node>() {}
    explicit ArgListNode(schema::HadronArgListNodeSchema* instance):
            NodeBase<ArgListNode, schema::HadronArgListNodeSchema, Node>(instance) {}
    explicit ArgListNode(Slot instance):
            NodeBase<ArgListNode, schema::HadronArgListNodeSchema, Node>(instance) {}
    ~ArgListNode() {}

    VarListNode varList() const { return VarListNode(m_instance->varList); }
    void setVarList(VarListNode v) { m_instance->varList = v.slot(); }

    Token varArgsNameToken() const { return Token(m_instance->varArgsNameToken); }
    void setVarArgsNameToken(Token tok) { m_instance->varArgsNameToken = tok.slot(); }
};

class ExprSeqNode : public NodeBase<ExprSeqNode, schema::HadronExprSeqNodeSchema, Node> {
public:
    ExprSeqNode(): NodeBase<ExprSeqNode, schema::HadronExprSeqNodeSchema, Node>() {}
    explicit ExprSeqNode(schema::HadronExprSeqNodeSchema* instance):
            NodeBase<ExprSeqNode, schema::HadronExprSeqNodeSchema, Node>(instance) {}
    explicit ExprSeqNode(Slot instance):
            NodeBase<ExprSeqNode, schema::HadronExprSeqNodeSchema, Node>(instance) {}
    ~ExprSeqNode() {}

    Node expr() const { return Node::wrapUnsafe(m_instance->expr); }
    void setExpr(Node n) { m_instance->expr = n.slot(); }
};

class ArrayReadNode : public NodeBase<ArrayReadNode, schema::HadronArrayReadNodeSchema, Node> {
public:
    ArrayReadNode(): NodeBase<ArrayReadNode, schema::HadronArrayReadNodeSchema, Node>() {}
    explicit ArrayReadNode(schema::HadronArrayReadNodeSchema* instance):
            NodeBase<ArrayReadNode, schema::HadronArrayReadNodeSchema, Node>(instance) {}
    explicit ArrayReadNode(Slot instance):
            NodeBase<ArrayReadNode, schema::HadronArrayReadNodeSchema, Node>(instance) {}
    ~ArrayReadNode() {}

    Node targetArray() const { return Node::wrapUnsafe(m_instance->targetArray); }
    void setTargetArray(Node n) { m_instance->targetArray = n.slot(); }

    ExprSeqNode indexArgument() const { return ExprSeqNode(m_instance->indexArgument); }
    void setIndexArgument(ExprSeqNode exprSeq) { m_instance->indexArgument = exprSeq.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_PARSE_NODE_HPP_