#ifndef SRC_HADRON_LIBRARY_HADRON_AST_HPP_
#define SRC_HADRON_LIBRARY_HADRON_AST_HPP_

#include "hadron/library/Array.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/schema/HLang/HadronASTSchema.hpp"

namespace hadron {
namespace library {

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

class NameAST : public ASTBase<NameAST, schema::HadronNameASTSchema> {
public:
    NameAST(): ASTBase<NameAST, schema::HadronNameASTSchema>() {}
    explicit NameAST(schema::HadronNameASTSchema* instance):
            ASTBase<NameAST, schema::HadronNameASTSchema>(instance) {}
    explicit NameAST(Slot instance): ASTBase<NameAST, schema::HadronNameASTSchema>(instance) {}
    ~NameAST() {}

    Symbol name(ThreadContext* context) const { return Symbol(context, m_instance->name); }
};

class AssignAST : public ASTBase<AssignAST, schema::HadronAssignASTSchema> {
public:
    AssignAST(): ASTBase<AssignAST, schema::HadronAssignASTSchema>() {}
    explicit AssignAST(schema::HadronAssignASTSchema* instance):
            ASTBase<AssignAST, schema::HadronAssignASTSchema>(instance) {}
    explicit AssignAST(Slot instance): ASTBase<AssignAST, schema::HadronAssignASTSchema>(instance) {}
    ~AssignAST() {}

    NameAST name() const { return NameAST(m_instance->name); }
    AST value() const { return AST::wrapUnsafe(m_instance->value); }
};

class BlockAST : public ASTBase<BlockAST, schema::HadronBlockASTSchema> {
public:
    BlockAST(): ASTBase<BlockAST, schema::HadronBlockASTSchema>() {}
    explicit BlockAST(schema::HadronBlockASTSchema* instance):
            ASTBase<BlockAST, schema::HadronBlockASTSchema>(instance) {}
    explicit BlockAST(Slot instance): ASTBase<BlockAST, schema::HadronBlockASTSchema>(instance) {}
    ~BlockAST() {}

    SymbolArray argumentNames() const { return SymbolArray(m_instance->argumentNames); }
    Array argumentDefaults() const { return Array(m_instance->argumentDefaults); }
    bool hasVarArg() const { return m_instance->hasVarArg.getBool(); }
    Array statements() const { return Array(m_instance->statements); }
};

class ConstantAST : public ASTBase<ConstantAST, schema::HadronConstantASTSchema> {
public:
    ConstantAST(): ASTBase<ConstantAST, schema::HadronConstantASTSchema>() {}
    explicit ConstantAST(schema::HadronConstantASTSchema* instance):
            ASTBase<ConstantAST, schema::HadronConstantASTSchema>(instance) {}
    explicit ConstantAST(Slot instance):
            ASTBase<ConstantAST, schema::HadronConstantASTSchema>(instance) {}
    ~ConstantAST() {}

    Slot constant() const { return m_instance->constant; }
};

class DefineAST : public ASTBase<DefineAST, schema::HadronDefineASTSchema> {
public:
    DefineAST(): ASTBase<DefineAST, schema::HadronDefineASTSchema>() {}
    explicit DefineAST(schema::HadronDefineASTSchema* instance):
            ASTBase<DefineAST, schema::HadronDefineASTSchema>(instance) {}
    explicit DefineAST(Slot instance): ASTBase<DefineAST, schema::HadronDefineASTSchema>(instance) {}
    ~DefineAST() {}

    NameAST name() const { return NameAST(m_instance->name); }
    AST value() const { return AST::wrapUnsafe(m_instance->value); }
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

    Array condition() const { return Array(m_instance->condition); }
    BlockAST trueBlock() const { return BlockAST(m_instance->trueBlock); }
    BlockAST falseBlock() const { return BlockAST(m_instance->falseBlock); }
};

class MessageAST : public ASTBase<MessageAST, schema::HadronMessageASTSchema> {
public:
    MessageAST(): ASTBase<MessageAST, schema::HadronMessageASTSchema>() {}
    explicit MessageAST(schema::HadronMessageASTSchema* instance):
            ASTBase<MessageAST, schema::HadronMessageASTSchema>(instance) {}
    explicit MessageAST(Slot instance): ASTBase<MessageAST, schema::HadronMessageASTSchema>(instance) {}
    ~MessageAST() {}

    Symbol selector(ThreadContext* context) const { return Symbol(context, m_instance->selector); }
    Array arguments() const { return Array(m_instance->arguments); }
    Array keywordArguments() const { return Array(m_instance->keywordArguments); }
};

class MethodReturnAST : public ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema> {
public:
    MethodReturnAST(): ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema>() {}
    explicit MethodReturnAST(schema::HadronMethodReturnASTSchema* instance):
            ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema>(instance) {}
    explicit MethodReturnAST(Slot instance): ASTBase<MethodReturnAST, schema::HadronMethodReturnASTSchema>(instance) {}
    ~MethodReturnAST() {}

    AST value() const { return AST::wrapUnsafe(m_instance->value); }
};

class MultiAssignAST : public ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema> {
public:
    MultiAssignAST(): ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema>() {}
    explicit MultiAssignAST(schema::HadronMultiAssignASTSchema* instance):
            ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema>(instance) {}
    explicit MultiAssignAST(Slot instance): ASTBase<MultiAssignAST, schema::HadronMultiAssignASTSchema>(instance) {}
    ~MultiAssignAST() {}

    AST arrayValue() const { return AST::wrapUnsafe(m_instance->arrayValue); }
    Array targetNames() const { return Array(m_instance->targetNames); }
};

class WhileAST : public ASTBase<WhileAST, schema::HadronWhileASTSchema> {
public:
    WhileAST(): ASTBase<WhileAST, schema::HadronWhileASTSchema>() {}
    explicit WhileAST(schema::HadronWhileASTSchema* instance):
            ASTBase<WhileAST, schema::HadronWhileASTSchema>(instance) {}
    explicit WhileAST(Slot instance): ASTBase<WhileAST, schema::HadronWhileASTSchema>(instance) {}
    ~WhileAST() {}

    BlockAST conditionBlock() const { return BlockAST(m_instance->conditionBlock); }
    BlockAST repeatBlock() const { return BlockAST(m_instance->repeatBlock); }
};

} // namespace library
} // namespace hadron

#endif // SRC_HADRON_LIBRARY_HADRON_AST_HPP_