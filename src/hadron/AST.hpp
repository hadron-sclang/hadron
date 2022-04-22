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
    kMultiAssign,
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

struct NameAST;

struct AssignAST : public AST {
    AssignAST(): AST(kAssign) {}
    virtual ~AssignAST() = default;

    std::unique_ptr<NameAST> name;
    std::unique_ptr<AST> value;
};

struct SequenceAST;

struct BlockAST : public AST {
    BlockAST(): AST(kBlock), hasVarArg(false), statements(std::make_unique<SequenceAST>()) {}
    virtual ~BlockAST() = default;

    library::SymbolArray argumentNames;
    library::Array argumentDefaults;
    bool hasVarArg;
    std::unique_ptr<SequenceAST> statements;
};

struct ConstantAST : public AST {
    ConstantAST(Slot c): AST(kConstant), constant(c) {}
    virtual ~ConstantAST() = default;

    Slot constant;
};

struct DefineAST : public AST {
    DefineAST(): AST(kDefine) {}
    virtual ~DefineAST() = default;

    std::unique_ptr<NameAST> name;
    std::unique_ptr<AST> value;
};

struct EmptyAST : public AST {
    EmptyAST(): AST(kEmpty) {}
    virtual ~EmptyAST() = default;
};

struct IfAST : public AST {
    IfAST(): AST(kIf), condition(std::make_unique<SequenceAST>()) {}
    virtual ~IfAST() = default;

    std::unique_ptr<SequenceAST> condition;
    std::unique_ptr<BlockAST> trueBlock;
    std::unique_ptr<BlockAST> falseBlock;
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

struct MethodReturnAST : public AST {
    MethodReturnAST(): AST(kMethodReturn) {}
    virtual ~MethodReturnAST() = default;

    std::unique_ptr<AST> value;
};

struct MultiAssignAST : public AST {
    MultiAssignAST(): AST(kMultiAssign), lastIsRemain(false) {}
    virtual ~MultiAssignAST() = default;

    // The value that should evaluate to an Array.
    std::unique_ptr<AST> arrayValue;
    // The in-order sequence of names or anonymous targets that receive the individual elements of arrayValue.
    std::list<std::unique_ptr<NameAST>> targetNames;
    // If true, the last element receives the rest of the array.
    bool lastIsRemain;
};

struct NameAST : public AST {
    NameAST(library::Symbol n): AST(kName), name(n) {}
    virtual ~NameAST() = default;

    library::Symbol name;
};

struct SequenceAST : public AST {
    SequenceAST(): AST(kSequence) {}
    virtual ~SequenceAST() = default;

    std::list<std::unique_ptr<AST>> sequence;
};

struct WhileAST : public AST {
    WhileAST(): AST(kWhile) {}

    std::unique_ptr<BlockAST> condition;
    std::unique_ptr<BlockAST> repeatBlock;
};

} // namespace ast
} // namespace hadron

#endif // SRC_HADRON_AST_HPP_