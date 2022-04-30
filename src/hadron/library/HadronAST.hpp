#ifndef SRC_HADRON_LIBRARY_HADRON_AST_HPP_
#define SRC_HADRON_LIBRARY_HADRON_AST_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronASTSchema.hpp"

namespace hadron {
namespace library {

class AST;

template<typename T, typename S>
class ASTBase : public Object<T, S> {
public:
    ASTBase(): Object<T, S>() {}
    explicit ASTBase(S* instance): Object<T, S>(instance) {}
    explicit ASTBase(Slot instance): Object<T, S>(instance) {}
    ~ASTBase() {}
};

class AST : public ASTBase<AST, schema::HadronASTSchema> {
public:
    AST(): ASTBase<AST, schema::HadronASTSchema>() {}
    explicit AST(schema::HadronASTSchema* instance): ASTBase<AST, schema::HadronASTSchema>(instance) {}
    explicit AST(Slot instance): ASTBase<AST, schema::HadronASTSchema>(instance) {}
    ~AST() {}
};

class AssignAST : public ASTBase<AssignAST, schema::HadronAssignASTSchema> {
public:
    AssignAST(): ASTBase<AssignAST, schema::HadronAssignASTSchema>() {}
    explicit AssignAST(schema::HadronAssignASTSchema* instance):
            ASTBase<AssignAST, schema::HadronAssignASTSchema>(instance) {}
    explicit AssignAST(Slot instance): ASTBase<AssignAST, schema::HadronAssignASTSchema>(instance) {}
    ~AssignAST() {}

    static inline AssignAST makeAssign(ThreadContext* context) {
        auto assignAST = AssignAST::alloc(context);
        assignAST.setName(Symbol());
        assignAST.setValue(AST());
        return assignAST;
    }

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol n) { m_instance->name = n.slot(); }

    AST value() const { return AST::wrapUnsafe(m_instance->value); }
    void setValue(AST a) { m_instance->value = a.slot(); }
};

class SequenceAST : public ASTBase<SequenceAST, schema::HadronSequenceASTSchema> {
public:
    SequenceAST(): ASTBase<SequenceAST, schema::HadronSequenceASTSchema>() {}
    explicit SequenceAST(schema::HadronSequenceASTSchema* instance):
            ASTBase<SequenceAST, schema::HadronSequenceASTSchema>(instance) {}
    explicit SequenceAST(Slot instance):
            ASTBase<SequenceAST, schema::HadronSequenceASTSchema>(instance) {}
    ~SequenceAST() {}

    static inline SequenceAST makeSequence(ThreadContext* context) {
        auto sequenceAST = SequenceAST::alloc(context);
        sequenceAST.setSequence(Array::arrayAlloc(context));
        return sequenceAST;
    }

    void addAST(ThreadContext* context, AST ast) { setSequence(sequence().add(context, ast.slot())); }

    Array sequence() const { return library::Array(m_instance->sequence); }
    void setSequence(Array a) { m_instance->sequence = a.slot(); }
};

class BlockAST : public ASTBase<BlockAST, schema::HadronBlockASTSchema> {
public:
    BlockAST(): ASTBase<BlockAST, schema::HadronBlockASTSchema>() {}
    explicit BlockAST(schema::HadronBlockASTSchema* instance):
            ASTBase<BlockAST, schema::HadronBlockASTSchema>(instance) {}
    explicit BlockAST(Slot instance): ASTBase<BlockAST, schema::HadronBlockASTSchema>(instance) {}
    ~BlockAST() {}

    static inline BlockAST makeBlock(ThreadContext* context) {
        auto blockAST = BlockAST::alloc(context);
        blockAST.setArgumentNames(SymbolArray::arrayAlloc(context));
        blockAST.setArgumentDefaults(Array::arrayAlloc(context));
        blockAST.setHasVarArg(false);
        blockAST.setStatements(SequenceAST::makeSequence(context));
        return blockAST;
    }

    SymbolArray argumentNames() const { return SymbolArray(m_instance->argumentNames); }
    void setArgumentNames(SymbolArray names) { m_instance->argumentNames = names.slot(); }

    Array argumentDefaults() const { return Array(m_instance->argumentDefaults); }
    void setArgumentDefaults(Array defaults) { m_instance->argumentDefaults = defaults.slot(); }

    bool hasVarArg() const { return m_instance->hasVarArg.getBool(); }
    void setHasVarArg(bool has) { m_instance->hasVarArg = Slot::makeBool(has); }

    SequenceAST statements() const { return SequenceAST(m_instance->statements); }
    void setStatements(SequenceAST s) { m_instance->statements = s.slot(); }
};

class ConstantAST : public ASTBase<ConstantAST, schema::HadronConstantASTSchema> {
public:
    ConstantAST(): ASTBase<ConstantAST, schema::HadronConstantASTSchema>() {}
    explicit ConstantAST(schema::HadronConstantASTSchema* instance):
            ASTBase<ConstantAST, schema::HadronConstantASTSchema>(instance) {}
    explicit ConstantAST(Slot instance):
            ASTBase<ConstantAST, schema::HadronConstantASTSchema>(instance) {}
    ~ConstantAST() {}

    static ConstantAST makeConstant(ThreadContext* context, Slot c) {
        auto constantAST = ConstantAST::alloc(context);
        constantAST.setConstant(c);
        return constantAST;
    }

    Slot constant() const { return m_instance->constant; }
    void setConstant(Slot c) { m_instance->constant = c; }
};

class DefineAST : public ASTBase<DefineAST, schema::HadronDefineASTSchema> {
public:
    DefineAST(): ASTBase<DefineAST, schema::HadronDefineASTSchema>() {}
    explicit DefineAST(schema::HadronDefineASTSchema* instance):
            ASTBase<DefineAST, schema::HadronDefineASTSchema>(instance) {}
    explicit DefineAST(Slot instance): ASTBase<DefineAST, schema::HadronDefineASTSchema>(instance) {}
    ~DefineAST() {}

    static DefineAST makeDefine(ThreadContext* context) {
        auto defineAST = DefineAST::alloc(context);
        defineAST.setName(Symbol());
        defineAST.setValue(AST());
        return defineAST;
    }

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol s) { m_instance->name = s.slot(); }

    AST value() const { return AST::wrapUnsafe(m_instance->value); }
    void setValue(AST a) { m_instance->value = a.slot(); }
};

class EmptyAST : public ASTBase<EmptyAST, schema::HadronEmptyASTSchema> {
public:
    EmptyAST(): ASTBase<EmptyAST, schema::HadronEmptyASTSchema>() {}
    explicit EmptyAST(schema::HadronEmptyASTSchema* instance):
            ASTBase<EmptyAST, schema::HadronEmptyASTSchema>(instance) {}
    explicit EmptyAST(Slot instance): ASTBase<EmptyAST, schema::HadronEmptyASTSchema>(instance) {}
    ~EmptyAST() {}
};

class IfAST : public ASTBase<IfAST, schema::HadronIfASTSchema> {
public:
    IfAST(): ASTBase<IfAST, schema::HadronIfASTSchema>() {}
    explicit IfAST(schema::HadronIfASTSchema* instance): ASTBase<IfAST, schema::HadronIfASTSchema>(instance) {}
    explicit IfAST(Slot instance): ASTBase<IfAST, schema::HadronIfASTSchema>(instance) {}
    ~IfAST() {}

    static inline IfAST makeIf(ThreadContext* context) {
        auto ifAST = IfAST::alloc(context);
        ifAST.setCondition(SequenceAST::makeSequence(context));
        ifAST.setTrueBlock(BlockAST::makeBlock(context));
        ifAST.setFalseBlock(BlockAST::makeBlock(context));
        return ifAST;
    }

    SequenceAST condition() const { return SequenceAST(m_instance->condition); }
    void setCondition(SequenceAST a) { m_instance->condition = a.slot(); }

    BlockAST trueBlock() const { return BlockAST(m_instance->trueBlock); }
    void setTrueBlock(BlockAST b) { m_instance->trueBlock = b.slot(); }

    BlockAST falseBlock() const { return BlockAST(m_instance->falseBlock); }
    void setFalseBlock(BlockAST b) { m_instance->falseBlock = b.slot(); }
};

class MessageAST : public ASTBase<MessageAST, schema::HadronMessageASTSchema> {
public:
    MessageAST(): ASTBase<MessageAST, schema::HadronMessageASTSchema>() {}
    explicit MessageAST(schema::HadronMessageASTSchema* instance):
            ASTBase<MessageAST, schema::HadronMessageASTSchema>(instance) {}
    explicit MessageAST(Slot instance): ASTBase<MessageAST, schema::HadronMessageASTSchema>(instance) {}
    ~MessageAST() {}

    static inline MessageAST makeMessage(ThreadContext* context) {
        auto messageAST = MessageAST::alloc(context);
        messageAST.setSelector(Symbol());
        messageAST.setArguments(SequenceAST::makeSequence(context));
        messageAST.setKeywordArguments(SequenceAST::makeSequence(context));
        return messageAST;
    }

    Symbol selector(ThreadContext* context) const { return Symbol(context, m_instance->selector); }
    void setSelector(Symbol s) { m_instance->selector = s.slot(); }

    SequenceAST arguments() const { return SequenceAST(m_instance->arguments); }
    void setArguments(SequenceAST a) { m_instance->arguments = a.slot(); }

    SequenceAST keywordArguments() const { return SequenceAST(m_instance->keywordArguments); }
    void setKeywordArguments(SequenceAST a) { m_instance->keywordArguments = a.slot(); }
};

class MethodReturnAST : public ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema> {
public:
    MethodReturnAST(): ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema>() {}
    explicit MethodReturnAST(schema::HadronMethodReturnASTSchema* instance):
            ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema>(instance) {}
    explicit MethodReturnAST(Slot instance): ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema>(instance) {}
    ~MethodReturnAST() {}

    static inline MethodReturnAST makeMethodReturn(ThreadContext* context) {
        auto methodReturnAST = MethodReturnAST::alloc(context);
        methodReturnAST.setValue(AST());
        return methodReturnAST;
    }

    AST value() const { return AST::wrapUnsafe(m_instance->value); }
    void setValue(AST v) { m_instance->value = v.slot(); }
};

class MultiAssignAST : public ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema> {
public:
    MultiAssignAST(): ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema>() {}
    explicit MultiAssignAST(schema::HadronMultiAssignASTSchema* instance):
            ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema>(instance) {}
    explicit MultiAssignAST(Slot instance): ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema>(instance) {}
    ~MultiAssignAST() {}

    static inline MultiAssignAST makeMultiAssign(ThreadContext* context) {
        auto multiAssignAST = MultiAssignAST::alloc(context);
        multiAssignAST.setArrayValue(AST());
        multiAssignAST.setTargetNames(SequenceAST::makeSequence(context));
        multiAssignAST.setLastIsRemain(false);
        return multiAssignAST;
    }

    AST arrayValue() const { return AST::wrapUnsafe(m_instance->arrayValue); }
    void setArrayValue(AST a) { m_instance->arrayValue = a.slot(); }

    SequenceAST targetNames() const { return SequenceAST(m_instance->targetNames); }
    void setTargetNames(SequenceAST s) { m_instance->targetNames = s.slot(); }

    bool lastIsRemain() const { return m_instance->lastIsRemain.getBool(); }
    void setLastIsRemain(bool b) { m_instance->lastIsRemain = Slot::makeBool(b); }
};

class NameAST : public ASTBase<NameAST, schema::HadronNameASTSchema> {
public:
    NameAST(): ASTBase<NameAST, schema::HadronNameASTSchema>() {}
    explicit NameAST(schema::HadronNameASTSchema* instance):
            ASTBase<NameAST, schema::HadronNameASTSchema>(instance) {}
    explicit NameAST(Slot instance): ASTBase<NameAST, schema::HadronNameASTSchema>(instance) {}
    ~NameAST() {}

    static inline NameAST makeName(ThreadContext* context, Symbol n) {
        auto nameAST = NameAST::alloc(context);
        nameAST.setName(n);
        return nameAST;
    }

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
    void setName(Symbol n) { m_instance->name = n.slot(); }
};

class WhileAST : public ASTBase<WhileAST, schema::HadronWhileASTSchema> {
public:
    WhileAST(): ASTBase<WhileAST, schema::HadronWhileASTSchema>() {}
    explicit WhileAST(schema::HadronWhileASTSchema* instance):
            ASTBase<WhileAST, schema::HadronWhileASTSchema>(instance) {}
    explicit WhileAST(Slot instance): ASTBase<WhileAST, schema::HadronWhileASTSchema>(instance) {}
    ~WhileAST() {}

    static inline WhileAST makeWhile(ThreadContext* context) {
        auto whileAST = WhileAST::alloc(context);
        whileAST.setConditionBlock(BlockAST::makeBlock(context));
        whileAST.setRepeatBlock(BlockAST::makeBlock(context));
        return whileAST;
    }

    BlockAST conditionBlock() const { return BlockAST(m_instance->conditionBlock); }
    void setConditionBlock(BlockAST b) { m_instance->conditionBlock = b.slot(); }

    BlockAST repeatBlock() const { return BlockAST(m_instance->repeatBlock); }
    void setRepeatBlock(BlockAST b) { m_instance->repeatBlock = b.slot(); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_AST_HPP_