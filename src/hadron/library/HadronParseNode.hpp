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

    Symbol snippet(ThreadContext* context) const { return Symbol(context, m_instance->snippet); }
    void setSnippet(Symbol s) { m_instance->snippet = s.slot(); }

    bool hasEscapeCharacters() const { return m_instance->hasEscapeCharacters.getBool(); }
    void setHasEscapeCharacters(bool b) { m_instance->hasEscapeCharacters = Slot::makeBool(b); }
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
        return Token(t.m_instance->token);
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
class CallNodeBase : public NodeBase<T, S, Node> {
public:
    CallNodeBase(): NodeBase<T, S, Node>() {}
    explicit CallNodeBase(S* instance): NodeBase<T, S, Node>(instance) {}
    explicit CallNodeBase(Slot instance): NodeBase<T, S, Node>(instance) {}
    ~CallNodeBase() {}

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
        t.m_instance->keywordArguments = n.slot();
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

    bool hasReadAccessor() const { return m_instance->hasReadAccessor.getBool(); }
    void setHasReadAccessor(bool b) { m_instance->hasReadAccessor = Slot::makeBool(b); }

    bool hasWriteAccessor() const { return m_instance->hasWriteAccessor.getBool(); }
    void setHasWriteAccessor(bool b) { m_instance->hasWriteAccessor = Slot::makeBool(b); }
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

class ArrayWriteNode : public NodeBase<ArrayWriteNode, schema::HadronArrayWriteNodeSchema, Node> {
public:
    ArrayWriteNode(): NodeBase<ArrayWriteNode, schema::HadronArrayWriteNodeSchema, Node>() {}
    explicit ArrayWriteNode(schema::HadronArrayWriteNodeSchema* instance):
            NodeBase<ArrayWriteNode, schema::HadronArrayWriteNodeSchema, Node>(instance) {}
    explicit ArrayWriteNode(Slot instance):
            NodeBase<ArrayWriteNode, schema::HadronArrayWriteNodeSchema, Node>(instance) {}
    ~ArrayWriteNode() {}

    Node targetArray() const { return Node::wrapUnsafe(m_instance->targetArray); }
    void setTargetArray(Node n) { m_instance->targetArray = n.slot(); }

    ExprSeqNode indexArgument() const { return ExprSeqNode(m_instance->indexArgument); }
    void setIndexArgument(ExprSeqNode exprSeq) { m_instance->indexArgument = exprSeq.slot(); }

    Node value() const { return Node::wrapUnsafe(m_instance->value); }
    void setValue(Node n) { m_instance->value = n.slot(); }
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
    void setConditionBlock(BlockNode n) { m_instance->conditionBlock = n.slot(); }

    BlockNode actionBlock() const { return BlockNode(m_instance->actionBlock); }
    void setActionBlock(BlockNode n) { m_instance->actionBlock = n.slot(); }
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

    NameNode className() const { return NameNode(m_instance->className); }
    void setClassName(NameNode nameNode) { m_instance->className = nameNode.slot(); }

    Node elements() const { return Node::wrapUnsafe(m_instance->elements); }
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

class MultiAssignNode : public NodeBase<MultiAssignNode, schema::HadronMultiAssignNodeSchema, Node> {
public:
    MultiAssignNode(): NodeBase<MultiAssignNode, schema::HadronMultiAssignNodeSchema, Node>() {}
    explicit MultiAssignNode(schema::HadronMultiAssignNodeSchema* instance):
            NodeBase<MultiAssignNode, schema::HadronMultiAssignNodeSchema, Node>(instance) {}
    explicit MultiAssignNode(Slot instance):
            NodeBase<MultiAssignNode, schema::HadronMultiAssignNodeSchema, Node>(instance) {}
    ~MultiAssignNode() {}

    MultiAssignVarsNode targets() const { return MultiAssignVarsNode(m_instance->targets); }
    void setTargets(MultiAssignVarsNode maVars) { m_instance->targets = maVars.slot(); }

    Node value() const { return Node::wrapUnsafe(m_instance->value); }
    void setValue(Node n) { m_instance->value = n.slot(); }
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

class PerformListNode : public CallNodeBase<PerformListNode, schema::HadronPerformListNodeSchema> {
public:
    PerformListNode(): CallNodeBase<PerformListNode, schema::HadronPerformListNodeSchema>() {}
    explicit PerformListNode(schema::HadronPerformListNodeSchema* instance):
            CallNodeBase<PerformListNode, schema::HadronPerformListNodeSchema>(instance) {}
    explicit PerformListNode(Slot instance):
            CallNodeBase<PerformListNode, schema::HadronPerformListNodeSchema>(instance) {}
    ~PerformListNode() {}
};

class CallNode : public CallNodeBase<CallNode, schema::HadronCallNodeSchema> {
public:
    CallNode(): CallNodeBase<CallNode, schema::HadronCallNodeSchema>() {}
    explicit CallNode(schema::HadronCallNodeSchema* instance):
            CallNodeBase<CallNode, schema::HadronCallNodeSchema>(instance) {}
    explicit CallNode(Slot instance):
            CallNodeBase<CallNode, schema::HadronCallNodeSchema>(instance) {}
    ~CallNode() {}
};

class NewNode : public CallNodeBase<NewNode, schema::HadronNewNodeSchema> {
public:
    NewNode(): CallNodeBase<NewNode, schema::HadronNewNodeSchema>() {}
    explicit NewNode(schema::HadronNewNodeSchema* instance):
            CallNodeBase<NewNode, schema::HadronNewNodeSchema>(instance) {}
    explicit NewNode(Slot instance):
            CallNodeBase<NewNode, schema::HadronNewNodeSchema>(instance) {}
    ~NewNode() {}
};

class ValueNode : public CallNodeBase<ValueNode, schema::HadronValueNodeSchema> {
public:
    ValueNode(): CallNodeBase<ValueNode, schema::HadronValueNodeSchema>() {}
    explicit ValueNode(schema::HadronValueNodeSchema* instance):
            CallNodeBase<ValueNode, schema::HadronValueNodeSchema>(instance) {}
    explicit ValueNode(Slot instance):
            CallNodeBase<ValueNode, schema::HadronValueNodeSchema>(instance) {}
    ~ValueNode() {}
};

class CurryArgumentNode : public NodeBase<CurryArgumentNode, schema::HadronCurryArgumentNodeSchema, Node> {
public:
    CurryArgumentNode(): NodeBase<CurryArgumentNode, schema::HadronCurryArgumentNodeSchema, Node>() {}
    explicit CurryArgumentNode(schema::HadronCurryArgumentNodeSchema* instance):
            NodeBase<CurryArgumentNode, schema::HadronCurryArgumentNodeSchema, Node>(instance) {}
    explicit CurryArgumentNode(Slot instance):
            NodeBase<CurryArgumentNode, schema::HadronCurryArgumentNodeSchema, Node>(instance) {}
    ~CurryArgumentNode() {}
};

class EnvironmentAtNode : public NodeBase<EnvironmentAtNode, schema::HadronEnvironmentAtNodeSchema, Node> {
public:
    EnvironmentAtNode(): NodeBase<EnvironmentAtNode, schema::HadronEnvironmentAtNodeSchema, Node>() {}
    explicit EnvironmentAtNode(schema::HadronEnvironmentAtNodeSchema* instance):
            NodeBase<EnvironmentAtNode, schema::HadronEnvironmentAtNodeSchema, Node>(instance) {}
    explicit EnvironmentAtNode(Slot instance):
            NodeBase<EnvironmentAtNode, schema::HadronEnvironmentAtNodeSchema, Node>(instance) {}
    ~EnvironmentAtNode() {}
};

class EnvironmentPutNode : public NodeBase<EnvironmentPutNode, schema::HadronEnvironmentPutNodeSchema, Node> {
public:
    EnvironmentPutNode(): NodeBase<EnvironmentPutNode, schema::HadronEnvironmentPutNodeSchema, Node>() {}
    explicit EnvironmentPutNode(schema::HadronEnvironmentPutNodeSchema* instance):
            NodeBase<EnvironmentPutNode, schema::HadronEnvironmentPutNodeSchema, Node>(instance) {}
    explicit EnvironmentPutNode(Slot instance):
            NodeBase<EnvironmentPutNode, schema::HadronEnvironmentPutNodeSchema, Node>(instance) {}
    ~EnvironmentPutNode() {}

    Node value() const { return Node::wrapUnsafe(m_instance->value); }
    void setValue(Node node) { m_instance->value = node.slot(); }
};

class CopySeriesNode : public NodeBase<CopySeriesNode, schema::HadronCopySeriesNodeSchema, Node> {
public:
    CopySeriesNode(): NodeBase<CopySeriesNode, schema::HadronCopySeriesNodeSchema, Node>() {}
    explicit CopySeriesNode(schema::HadronCopySeriesNodeSchema* instance):
            NodeBase<CopySeriesNode, schema::HadronCopySeriesNodeSchema, Node>(instance) {}
    explicit CopySeriesNode(Slot instance):
            NodeBase<CopySeriesNode, schema::HadronCopySeriesNodeSchema, Node>(instance) {}
    ~CopySeriesNode() {}

    Node target() const { return Node::wrapUnsafe(m_instance->target); }
    void setTarget(Node node) { m_instance->target = node.slot(); }

    ExprSeqNode first() const { return ExprSeqNode(m_instance->first); }
    void setFirst(ExprSeqNode exprSeq) { m_instance->first = exprSeq.slot(); }

    Node second() const { return Node::wrapUnsafe(m_instance->second); }
    void setSecond(Node node) { m_instance->second = node.slot(); }

    ExprSeqNode last() const { return ExprSeqNode(m_instance->last); }
    void setLast(ExprSeqNode exprSeq) { m_instance->last = exprSeq.slot(); }
};

class BinopCallNode : public NodeBase<BinopCallNode, schema::HadronBinopCallNodeSchema, Node> {
public:
    BinopCallNode(): NodeBase<BinopCallNode, schema::HadronBinopCallNodeSchema, Node>() {}
    explicit BinopCallNode(schema::HadronBinopCallNodeSchema* instance):
            NodeBase<BinopCallNode, schema::HadronBinopCallNodeSchema, Node>(instance) {}
    explicit BinopCallNode(Slot instance):
            NodeBase<BinopCallNode, schema::HadronBinopCallNodeSchema, Node>(instance) {}
    ~BinopCallNode() {}

    Node leftHand() const { return Node::wrapUnsafe(m_instance->leftHand); }
    void setLeftHand(Node node) { m_instance->leftHand = node.slot(); }

    Node rightHand() const { return Node::wrapUnsafe(m_instance->rightHand); }
    void setRightHand(Node node) { m_instance->rightHand = node.slot(); }

    Node adverb() const { return Node::wrapUnsafe(m_instance->adverb); }
    void setAdverb(Node node) { m_instance->adverb = node.slot(); }
};

class AssignNode : public NodeBase<AssignNode, schema::HadronAssignNodeSchema, Node> {
public:
    AssignNode(): NodeBase<AssignNode, schema::HadronAssignNodeSchema, Node>() {}
    explicit AssignNode(schema::HadronAssignNodeSchema* instance):
            NodeBase<AssignNode, schema::HadronAssignNodeSchema, Node>(instance) {}
    explicit AssignNode(Slot instance):
            NodeBase<AssignNode, schema::HadronAssignNodeSchema, Node>(instance) {}
    ~AssignNode() {}

    NameNode name() const { return NameNode(m_instance->name); }
    void setName(NameNode n) { m_instance->name = n.slot(); }

    Node value() const { return Node::wrapUnsafe(m_instance->value); }
    void setValue(Node v) { m_instance->value = v.slot(); }
};

class SetterNode : public NodeBase<SetterNode, schema::HadronSetterNodeSchema, Node> {
public:
    SetterNode(): NodeBase<SetterNode, schema::HadronSetterNodeSchema, Node>() {}
    explicit SetterNode(schema::HadronSetterNodeSchema* instance):
            NodeBase<SetterNode, schema::HadronSetterNodeSchema, Node>(instance) {}
    explicit SetterNode(Slot instance):
            NodeBase<SetterNode, schema::HadronSetterNodeSchema, Node>(instance) {}
    ~SetterNode() {}

    Node target() const { return Node::wrapUnsafe(m_instance->target); }
    void setTarget(Node n) { m_instance->target = n.slot(); }

    Node value() const { return Node::wrapUnsafe(m_instance->value); }
    void setValue(Node n) { m_instance->value = n.slot(); }
};

class SlotNode : public NodeBase<SlotNode, schema::HadronSlotNodeSchema, Node> {
public:
    SlotNode(): NodeBase<SlotNode, schema::HadronSlotNodeSchema, Node>() {}
    explicit SlotNode(schema::HadronSlotNodeSchema* instance):
            NodeBase<SlotNode, schema::HadronSlotNodeSchema, Node>(instance) {}
    explicit SlotNode(Slot instance):
            NodeBase<SlotNode, schema::HadronSlotNodeSchema, Node>(instance) {}
    ~SlotNode() {}

    Slot value() const { return m_instance->value; }
    void setValue(Slot s) { m_instance->value = s; }
};

class EmptyNode : public NodeBase<EmptyNode, schema::HadronEmptyNodeSchema, Node> {
public:
    EmptyNode(): NodeBase<EmptyNode, schema::HadronEmptyNodeSchema, Node>() {}
    explicit EmptyNode(schema::HadronEmptyNodeSchema* instance):
            NodeBase<EmptyNode, schema::HadronEmptyNodeSchema, Node>(instance) {}
    explicit EmptyNode(Slot instance):
            NodeBase<EmptyNode, schema::HadronEmptyNodeSchema, Node>(instance) {}
    ~EmptyNode() {}
};

class ListCompNode : public NodeBase<ListCompNode, schema::HadronListComprehensionNodeSchema, Node> {
public:
    ListCompNode(): NodeBase<ListCompNode, schema::HadronListComprehensionNodeSchema, Node>() {}
    explicit ListCompNode(schema::HadronListComprehensionNodeSchema* instance):
            NodeBase<ListCompNode, schema::HadronListComprehensionNodeSchema, Node>(instance) {}
    explicit ListCompNode(Slot instance):
            NodeBase<ListCompNode, schema::HadronListComprehensionNodeSchema, Node>(instance) {}
    ~ListCompNode() {}

    ExprSeqNode body() const { return ExprSeqNode(m_instance->body); }
    void setBody(ExprSeqNode seq) { m_instance->body = seq.slot(); }

    Node qualifiers() const { return Node::wrapUnsafe(m_instance->qualifiers); }
    void setQualifiers(Node n) { m_instance->qualifiers = n.slot(); }
};

class TerminationQualNode : public NodeBase<TerminationQualNode, schema::HadronTerminationQualifierNodeSchema, Node> {
public:
    TerminationQualNode(): NodeBase<TerminationQualNode, schema::HadronTerminationQualifierNodeSchema, Node>() {}
    explicit TerminationQualNode(schema::HadronTerminationQualifierNodeSchema* instance):
            NodeBase<TerminationQualNode, schema::HadronTerminationQualifierNodeSchema, Node>(instance) {}
    explicit TerminationQualNode(Slot instance):
            NodeBase<TerminationQualNode, schema::HadronTerminationQualifierNodeSchema, Node>(instance) {}
    ~TerminationQualNode() {}

    ExprSeqNode exprSeq() const { return ExprSeqNode(m_instance->exprSeq); }
    void setExprSeq(ExprSeqNode seq) { m_instance->exprSeq = seq.slot(); }
};

class SideEffectQualNode : public NodeBase<SideEffectQualNode, schema::HadronSideEffectQualifierNodeSchema, Node> {
public:
    SideEffectQualNode(): NodeBase<SideEffectQualNode, schema::HadronSideEffectQualifierNodeSchema, Node>() {}
    explicit SideEffectQualNode(schema::HadronSideEffectQualifierNodeSchema* instance):
            NodeBase<SideEffectQualNode, schema::HadronSideEffectQualifierNodeSchema, Node>(instance) {}
    explicit SideEffectQualNode(Slot instance):
            NodeBase<SideEffectQualNode, schema::HadronSideEffectQualifierNodeSchema, Node>(instance) {}
    ~SideEffectQualNode() {}

    ExprSeqNode exprSeq() const { return ExprSeqNode(m_instance->exprSeq); }
    void setExprSeq(ExprSeqNode seq) { m_instance->exprSeq = seq.slot(); }
};

class BindingQualNode : public NodeBase<BindingQualNode, schema::HadronBindingQualifierNodeSchema, Node> {
public:
    BindingQualNode(): NodeBase<BindingQualNode, schema::HadronBindingQualifierNodeSchema, Node>() {}
    explicit BindingQualNode(schema::HadronBindingQualifierNodeSchema* instance):
            NodeBase<BindingQualNode, schema::HadronBindingQualifierNodeSchema, Node>(instance) {}
    explicit BindingQualNode(Slot instance):
            NodeBase<BindingQualNode, schema::HadronBindingQualifierNodeSchema, Node>(instance) {}
    ~BindingQualNode() {}

    NameNode name() const { return NameNode(m_instance->name); }
    void setName(NameNode n) { m_instance->name = n.slot(); }

    ExprSeqNode exprSeq() const { return ExprSeqNode(m_instance->exprSeq); }
    void setExprSeq(ExprSeqNode seq) { m_instance->exprSeq = seq.slot(); }
};

class GuardQualNode : public NodeBase<GuardQualNode, schema::HadronGuardQualifierNodeSchema, Node> {
public:
    GuardQualNode(): NodeBase<GuardQualNode, schema::HadronGuardQualifierNodeSchema, Node>() {}
    explicit GuardQualNode(schema::HadronGuardQualifierNodeSchema* instance):
            NodeBase<GuardQualNode, schema::HadronGuardQualifierNodeSchema, Node>(instance) {}
    explicit GuardQualNode(Slot instance):
            NodeBase<GuardQualNode, schema::HadronGuardQualifierNodeSchema, Node>(instance) {}
    ~GuardQualNode() {}

    ExprSeqNode exprSeq() const { return ExprSeqNode(m_instance->exprSeq); }
    void setExprSeq(ExprSeqNode seq) { m_instance->exprSeq = seq.slot(); }
};

class GenQualNode : public NodeBase<GenQualNode, schema::HadronGeneratorQualifierNodeSchema, Node> {
public:
    GenQualNode(): NodeBase<GenQualNode, schema::HadronGeneratorQualifierNodeSchema, Node>() {}
    explicit GenQualNode(schema::HadronGeneratorQualifierNodeSchema* instance):
            NodeBase<GenQualNode, schema::HadronGeneratorQualifierNodeSchema, Node>(instance) {}
    explicit GenQualNode(Slot instance):
            NodeBase<GenQualNode, schema::HadronGeneratorQualifierNodeSchema, Node>(instance) {}
    ~GenQualNode() {}

    NameNode name() const { return NameNode(m_instance->name); }
    void setName(NameNode n) { m_instance->name = n.slot(); }

    ExprSeqNode exprSeq() const { return ExprSeqNode(m_instance->exprSeq); }
    void setExprSeq(ExprSeqNode seq) { m_instance->exprSeq = seq.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_PARSE_NODE_HPP_