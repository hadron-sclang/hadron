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
        return B::wrapUnsafe(Slot::makePointer(reinterpret_cast<library::Schema*>(m_instance)));
    }

    Token token() const { return Token(m_instance->token.slot()); }
    void setToken(Token t) { m_instance->token = t.slot(); }

    B next() const { return B::wrapUnsafe(m_instance->next); }
    void setNext(B b) { m_instance->next = b.slot(); }
};

class Node : public NodeBase<Node, schema::HadronParseNodeSchema, Node> {
public:
    Node(): NodeBase<Node, schema::HadronParseNodeSchema, Node>() {}
    explicit Node(schema::HadronParseNodeSchema* instance):
            NodeBase<Node, schema::HadronParseNodeSchema, Node>(instance) {}
    explicit Node(Slot instance): NodeBase<Node, schema::HadronParseNodeSchema, Node>(instance) {}
    ~Node() {}
};

class KeyValueNode : public NodeBase<KeyValueNode, schema::HadronParseKeyValueNodeSchema, Node>() {
public:
};

template<typename T, typename S>
class CallBaseNode : public NodeBase<T, S, Node> {
public:
    CallBaseNode(): NodeBase<T, S, Node>() {}
    explicit CallBaseNode(S* instance): NodeBase<T, S, Node>(instance) {}
    explicit CallBaseNode(Slot instance): NodeBase<T, S, Node>(instance) {}
    ~CallBaseNode() {}

    Node target() const { return Node::wrapUnsafe(m_instance->target); }
    void setTarget(Node n) { m_instance->target = n.slot(); }

    Node arguments() const { return Node::wrapUnsafe(m_instance->arguments); }
    void setArguments(Node n) { m_instance->arguments = n.slot(); }

    KeyValueNode keywordArguments() const { return KeyValueNode(m_instance->keywordArguments); }
    void setKeywordArguments(KeyValueNode n) { m_instance->keywordArguemnts = n.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_PARSE_NODE_HPP_