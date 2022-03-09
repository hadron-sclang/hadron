#ifndef SRC_HADRON_AST_HPP_
#define SRC_HADRON_AST_HPP_

#include "hadron/Slot.hpp"
#include "hadron/library/Array.hpp"
#include "hadron/library/Symbol.hpp"

#include <list>

namespace hadron {
namespace ast {

enum ASTType {
    kAssign,
    kBlock,
    kConstant,
    kDefine,
    kEmpty,
    kIf,
    kMessage,
    kMethodReturn,
    kName,
    kSequence,
    kWhile
};

struct AST {
    AST() = delete;
    virtual ~AST() = default;

    ASTType astType;

protected:
    AST(ASTType type): astType(type) {}
};

struct EmptyAST : public AST {
    EmptyAST(): AST(kEmpty) {}
    virtual ~EmptyAST() = default;
};

struct SequenceAST : public AST {
    SequenceAST(): AST(kSequence) {}
    virtual ~SequenceAST() = default;

    std::list<std::unique_ptr<AST>> sequence;
};

struct BlockAST : public AST {
    BlockAST(): AST(kBlock), hasVarArg(false), statements(std::make_unique<SequenceAST>()) {}
    virtual ~BlockAST() = default;

    library::SymbolArray argumentNames;
    library::Array argumentDefaults;
    bool hasVarArg;
    std::unique_ptr<SequenceAST> statements;
};

struct IfAST : public AST {
    IfAST(): AST(kIf), condition(std::make_unique<SequenceAST>()) {}
    virtual ~IfAST() = default;

    std::unique_ptr<SequenceAST> condition;
    std::unique_ptr<BlockAST> trueBlock;
    std::unique_ptr<BlockAST> falseBlock;
};

struct WhileAST : public AST {
    WhileAST(): AST(kWhile) {}

    std::unique_ptr<BlockAST> condition;
    std::unique_ptr<BlockAST> repeatBlock;
};

struct MessageAST : public AST {
    MessageAST(): AST(kMessage),
        arguments(std::make_unique<SequenceAST>()),
        keywordArguments(std::make_unique<SequenceAST>()) {}
    virtual ~MessageAST() = default;

    library::Symbol selector;
    std::unique_ptr<SequenceAST> arguments;
    std::unique_ptr<SequenceAST> keywordArguments;
};

struct NameAST : public AST {
    NameAST(library::Symbol n): AST(kName), name(n) {}
    virtual ~NameAST() = default;

    library::Symbol name;
};

struct DefineAST : public AST {
    DefineAST(): AST(kDefine) {}
    virtual ~DefineAST() = default;

    std::unique_ptr<NameAST> name;
    std::unique_ptr<AST> value;
};

struct AssignAST : public AST {
    AssignAST(): AST(kAssign) {}
    virtual ~AssignAST() = default;

    std::unique_ptr<NameAST> name;
    std::unique_ptr<AST> value;
};

struct ConstantAST : public AST {
    ConstantAST(Slot c): AST(kConstant), constant(c) {}
    virtual ~ConstantAST() = default;

    Slot constant;
};

struct MethodReturnAST : public AST {
    MethodReturnAST(): AST(kMethodReturn) {}
    virtual ~MethodReturnAST() = default;

    std::unique_ptr<AST> value;
};

} // namespace ast
} // namespace hadron

#endif // SRC_HADRON_AST_HPP_