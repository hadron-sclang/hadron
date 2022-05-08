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

    static T make(ThreadContext* context, Token tok) {
        auto t = T::alloc(context);
        t.initToNil();
        t.setToken(tok);
        t.setTail(t.toBase());
        return t;
    }

    B toBase() const {
        const T& t = static_cast<const T&>(*this);
        return B::wrapUnsafe(Slot::makePointer(reinterpret_cast<library::Schema*>(t.m_instance)));
    }

    void append(B node) {
        T& t = static_cast<T&>(*this);
        t.tail().setNext(node);
        t.setTail(tail().next().tail());
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

    B tail() const {
        const T& t = static_cast<const T&>(*this);
        return B::wrapUnsafe(t.m_instance->tail);
    }
    void setTail(B b) {
        T& t = static_cast<T&>(*this);
        t.m_instance->tail = b.slot();
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
            NodeBase<KeyValueNode, schema::HadronKeyValueNodeSchema, Node>(instance) {}
    explicit KeyValueNode(Slot instance):
            NodeBase<KeyValueNode, schema::HadronKeyValueNodeSchema, Node>(instance) {}
    ~KeyValueNode() {}

    Node key() const { return Node::wrapUnsafe(m_instance->key); }
    void setKey(Node n) { m_instance->key = n.slot(); }

    Node value() const { return Node::wrapUnsafe(m_instance->value); }
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

class BlockNode : public NodeBase<BlockNode, schema::HadronBlockNodeSchema, Node> {
public:
    BlockNode(): NodeBase<BlockNode, schema::HadronBlockNodeSchema, Node>() {}
    explicit BlockNode(schema::HadronBlockNodeSchema* instance):
            NodeBase<BlockNode, schema::HadronBlockNodeSchema, Node>(instance) {}
    explicit BlockNode(Slot instance):
            NodeBase<BlockNode, schema::HadronBlockNodeSchema, Node>(instance) {}
    ~BlockNode() {}

    ArgListNode arguments() const { return ArgListNode(m_instance->arguments); }
    void setArguments(ArgListNode argList) { m_instance->arguments = argList.slot(); }

    VarListNode variables() const { return VarListNode(m_instance->variables); }
    void setVariables(VarListNode varList) { m_instance->variables = varList.slot(); }

    ExprSeqNode body() const { return ExprSeqNode(m_instance->body); }
    void setBody(ExprSeqNode exprSeq) { m_instance->body = exprSeq.slot(); }
};

class MethodNode : public NodeBase<MethodNode, schema::HadronMethodNodeSchema, Node> {
public:
    MethodNode(): NodeBase<MethodNode, schema::HadronMethodNodeSchema, Node>() {}
    explicit MethodNode(schema::HadronMethodNodeSchema* instance):
            NodeBase<MethodNode, schema::HadronMethodNodeSchema, Node>(instance) {}
    explicit MethodNode(Slot instance):
            NodeBase<MethodNode, schema::HadronMethodNodeSchema, Node>(instance) {}
    ~MethodNode() {}

    bool isClassMethod() const { return m_instance->isClassMethod.getBool(); }
    void setIsClassMethod(bool isClassMethod) { m_instance->isClassMethod = Slot::makeBool(isClassMethod); }

    Token primitiveToken() const { return Token(m_instance->primitiveToken); }
    void setPrimitiveToken(Token tok) { m_instance->primitiveToken = tok.slot(); }

    BlockNode body() const { return BlockNode(m_instance->body); }
    void setBody(BlockNode blockNode) { m_instance->body = blockNode.slot(); }
};

class ClassNode : public NodeBase<ClassNode, schema::HadronClassNodeSchema, Node> {
public:
    ClassNode(): NodeBase<ClassNode, schema::HadronClassNodeSchema, Node>() {}
    explicit ClassNode(schema::HadronClassNodeSchema* instance):
            NodeBase<ClassNode, schema::HadronClassNodeSchema, Node>(instance) {}
    explicit ClassNode(Slot instance):
            NodeBase<ClassNode, schema::HadronClassNodeSchema, Node>(instance) {}
    ~ClassNode() {}

    Token superclassNameToken() const { return Token(m_instance->superclassNameToken); }
    void setSuperclassNameToken(Token tok) { m_instance->superclassNameToken = tok.slot(); }

    Token optionalNameToken() const { return Token(m_instance->optionalNameToken); }
    void setOptionalNameToken(Token tok) { m_instance->optionalNameToken = tok.slot(); }

    VarListNode variables() const { return VarListNode(m_instance->variables); }
    void setVariables(VarListNode varList) { m_instance->variables = varList.slot(); }

    MethodNode methods() const { return MethodNode(m_instance->methods); }
    void setMethods(MethodNode methodNode) { m_instance->methods = methodNode.slot(); }
};

class ClassExtNode : public NodeBase<ClassExtNode, schema::HadronClassExtensionNodeSchema, Node> {
public:
    ClassExtNode(): NodeBase<ClassExtNode, schema::HadronClassExtensionNodeSchema, Node>() {}
    explicit ClassExtNode(schema::HadronClassExtensionNodeSchema* instance):
            NodeBase<ClassExtNode, schema::HadronClassExtensionNodeSchema, Node>(instance) {}
    explicit ClassExtNode(Slot instance):
            NodeBase<ClassExtNode, schema::HadronClassExtensionNodeSchema, Node>(instance) {}
    ~ClassExtNode() {}

    MethodNode methods() const { return MethodNode(m_instance->methods); }
    void setMethods(MethodNode methodNode) { m_instance->methods = methodNode.slot(); }
};

class IfNode : public NodeBase<IfNode, schema::HadronIfNodeSchema, Node> {
public:
    IfNode(): NodeBase<IfNode, schema::HadronIfNodeSchema, Node>() {}
    explicit IfNode(schema::HadronIfNodeSchema* instance):
            NodeBase<IfNode, schema::HadronIfNodeSchema, Node>(instance) {}
    explicit IfNode(Slot instance): NodeBase<IfNode, schema::HadronIfNodeSchema, Node>(instance) {}
    ~IfNode() {}

    ExprSeqNode condition() const { return ExprSeqNode(m_instance->condition); }
    void setCondition(ExprSeqNode exprSeq) { m_instance->condition = exprSeq.slot(); }

    BlockNode trueBlock() const { return BlockNode(m_instance->trueBlock); }
    void setTrueBlock(BlockNode blockNode) { m_instance->trueBlock = blockNode.slot(); }

    BlockNode elseBlock() const { return BlockNode(m_instance->elseBlock); }
    void setElseBlock(BlockNode blockNode) { m_instance->elseBlock = blockNode.slot(); }
};

class WhileNode : public NodeBase<WhileNode, schema::HadronWhileNodeSchema, Node> {
public:
    WhileNode(): NodeBase<WhileNode, schema::HadronWhileNodeSchema, Node>() {}
    explicit WhileNode(schema::HadronWhileNodeSchema* instance):
            NodeBase<WhileNode, schema::HadronWhileNodeSchema, Node>(instance) {}
    explicit WhileNode(Slot instance):
            NodeBase<WhileNode, schema::HadronWhileNodeSchema, Node>(instance) {}
    ~WhileNode() {}

    BlockNode conditionBlock() const { return BlockNode(m_instance->conditionBlock); }
    void setConditionBlock(BlockNode blockNode) { m_instance->conditionBlock = blockNode.slot(); }

    BlockNode actionBlock() const { return BlockNode(m_instance->actionBlock); }
    void setActionBlock(BlockNode blockNode) { m_instance->actionBlock = blockNode.slot(); }
};

class EventNode : public NodeBase<EventNode, schema::HadronEventNodeSchema, Node> {
public:
    EventNode(): NodeBase<EventNode, schema::HadronEventNodeSchema, Node>() {}
    explicit EventNode(schema::HadronEventNodeSchema* instance):
            NodeBase<EventNode, schema::HadronEventNodeSchema, Node>(instance) {}
    explicit EventNode(Slot instance): NodeBase<EventNode, schema::HadronEventNodeSchema, Node>(instance) {}
    ~EventNode() {}

    Node elements() const { return Node::wrapUnsafe(m_instance->elements); }
    void setElements(Node node) { m_instance->elements = node.slot(); }
};

class NameNode : public NodeBase<NameNode, schema::HadronNameNodeSchema, Node> {
public:
    NameNode(): NodeBase<NameNode, schema::HadronNameNodeSchema, Node>() {}
    explicit NameNode(schema::HadronNameNodeSchema* instance):
            NodeBase<NameNode, schema::HadronNameNodeSchema, Node>(instance) {}
    explicit NameNode(Slot instance):
            NodeBase<NameNode, schema::HadronNameNodeSchema, Node>(instance) {}
    ~NameNode() {}
};

class CollectionNode : public NodeBase<CollectionNode, schema::HadronCollectionNodeSchema, Node> {
public:
    CollectionNode(): NodeBase<CollectionNode, schema::HadronCollectionNodeSchema, Node>() {}
    explicit CollectionNode(schema::HadronCollectionNodeSchema* instance):
            NodeBase<CollectionNode, schema::HadronCollectionNodeSchema, Node>(instance) {}
    explicit CollectionNode(Slot instance):
            NodeBase<CollectionNode, schema::HadronCollectionNodeSchema, Node>(instance) {}
    ~CollectionNode() {}

    NameNode className() const { NameNode(m_instance->className); }
    void setClassName(NameNode nameNode) { m_instance->className = nameNode.slot(); }

    Node elements() const { Node::wrapUnsafe(m_instance->elements); }
    void setElements(Node node) { m_instance->elements = node.slot(); }
};

class MultiAssignVarsNode : public NodeBase<MultiAssignVarsNode, schema::HadronMultiAssignVarsNodeSchema, Node> {
public:
    MultiAssignVarsNode(): NodeBase<MultiAssignVarsNode, schema::HadronMultiAssignVarsNodeSchema, Node>() {}
    explicit MultiAssignVarsNode(schema::HadronMultiAssignVarsNodeSchema* instance):
            NodeBase<MultiAssignVarsNode, schema::HadronMultiAssignVarsNodeSchema, Node>(instance) {}
    explicit MultiAssignVarsNode(Slot instance):
            NodeBase<MultiAssignVarsNode, schema::HadronMultiAssignVarsNodeSchema, Node>(instance) {}
    ~MultiAssignVarsNode() {}

    NameNode names() const { return NameNode(m_instance->names); }
    void setNames(NameNode nameNode) { m_instance->names = nameNode.slot(); }

    NameNode rest() const { return NameNode(m_instance->rest); }
    void setRest(NameNode nameNode) { m_instance->rest = nameNode.slot(); }
};

class ReturnNode : public NodeBase<ReturnNode, schema::HadronReturnNodeSchema, Node> {
public:
    ReturnNode(): NodeBase<ReturnNode, schema::HadronReturnNodeSchema, Node>() {}
    explicit ReturnNode(schema::HadronReturnNodeSchema* instance):
            NodeBase<ReturnNode, schema::HadronReturnNodeSchema, Node>(instance) {}
    explicit ReturnNode(Slot instance):
            NodeBase<ReturnNode, schema::HadronReturnNodeSchema, Node>(instance) {}
    ~ReturnNode() {}

    Node valueExpr() const { return Node::wrapUnsafe(m_instance->valueExpr); }
    void setValueExpr(Node node) { m_instance->valueExpr = node.slot(); }
};

class SeriesNode : public NodeBase<SeriesNode, schema::HadronSeriesNodeSchema, Node> {
public:
    SeriesNode(): NodeBase<SeriesNode, schema::HadronSeriesNodeSchema, Node>() {}
    explicit SeriesNode(schema::HadronSeriesNodeSchema* instance):
            NodeBase<SeriesNode, schema::HadronSeriesNodeSchema, Node>(instance) {}
    explicit SeriesNode(Slot instance):
            NodeBase<SeriesNode, schema::HadronSeriesNodeSchema, Node>(instance) {}
    ~SeriesNode() {}

    ExprSeqNode start() const { return ExprSeqNode(m_instance->start); }
    void setStart(ExprSeqNode exprSeq) { m_instance->start = exprSeq.slot(); }

    ExprSeqNode step() const { return ExprSeqNode(m_instance->step); }
    void setStep(ExprSeqNode exprSeq) { m_instance->step = exprSeq.slot(); }

    ExprSeqNode last() const { return ExprSeqNode(m_instance->last); }
    void setLast(ExprSeqNode exprSeq) { m_instance->last = exprSeq.slot(); }
};

class SeriesIterNode : public NodeBase<SeriesIterNode, schema::HadronSeriesIterNodeSchema, Node> {
public:
    SeriesIterNode(): NodeBase<SeriesIterNode, schema::HadronSeriesIterNodeSchema, Node>() {}
    explicit SeriesIterNode(schema::HadronSeriesIterNodeSchema* instance):
            NodeBase<SeriesIterNode, schema::HadronSeriesIterNodeSchema, Node>(instance) {}
    explicit SeriesIterNode(Slot instance):
            NodeBase<SeriesIterNode, schema::HadronSeriesIterNodeSchema, Node>(instance) {}
    ~SeriesIterNode() {}

    ExprSeqNode start() const { return ExprSeqNode(m_instance->start); }
    void setStart(ExprSeqNode exprSeq) { m_instance->start = exprSeq.slot(); }

    ExprSeqNode step() const { return ExprSeqNode(m_instance->step); }
    void setStep(ExprSeqNode exprSeq) { m_instance->step = exprSeq.slot(); }

    ExprSeqNode last() const { return ExprSeqNode(m_instance->last); }
    void setLast(ExprSeqNode exprSeq) { m_instance->last = exprSeq.slot(); }
};

class StringNode : public NodeBase<StringNode, schema::HadronStringNodeSchema, Node> {
public:
    StringNode(): NodeBase<StringNode, schema::HadronStringNodeSchema, Node>() {}
    explicit StringNode(schema::HadronStringNodeSchema* instance):
            NodeBase<StringNode, schema::HadronStringNodeSchema, Node>(instance) {}
    explicit StringNode(Slot instance):
            NodeBase<StringNode, schema::HadronStringNodeSchema, Node>(instance) {}
    ~StringNode() {}
};

class SymbolNode : public NodeBase<SymbolNode, schema::HadronSymbolNodeSchema, Node> {
public:
    SymbolNode(): NodeBase<SymbolNode, schema::HadronSymbolNodeSchema, Node>() {}
    explicit SymbolNode(schema::HadronSymbolNodeSchema* instance):
            NodeBase<SymbolNode, schema::HadronSymbolNodeSchema, Node>(instance) {}
    explicit SymbolNode(Slot instance):
            NodeBase<SymbolNode, schema::HadronSymbolNodeSchema, Node>(instance) {}
    ~SymbolNode() {}
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_PARSE_NODE_HPP_