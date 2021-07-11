#include "hadron/CodeGenerator.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/JIT.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "Keywords.hpp"
#include "RegisterAllocator.hpp"

namespace hadron {

CodeGenerator::CodeGenerator() {
    m_errorReporter = std::make_shared<ErrorReporter>(true);
}

CodeGenerator::CodeGenerator(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) {}

bool CodeGenerator::jitBlock(const ast::BlockAST* block, JIT* jit) {
    jit->prolog();
    RegisterAllocator allocator(block, jit);
    allocator.setupStack();
    for (const auto& statement : block->statements) {
        jitAST(statement.get(), jit, &allocator);
    }
    jit->epilog();
    return true;
}

void CodeGenerator::jitAST(const ast::AST* ast, JIT* jit, RegisterAllocator* allocator) {
    switch (ast->astType) {
    // No type checking right now, none at all
    case hadron::ast::ASTType::kAssign: {
        const auto assign = reinterpret_cast<const hadron::ast::AssignAST*>(ast);
        JIT::Reg targetReg = allocator->allocate(assign->target->registerNumber);
        if (assign->value->astType == hadron::ast::ASTType::kValue) {
            const auto target = reinterpret_cast<const hadron::ast::ValueAST*>(assign->value.get());
            JIT::Reg valueReg = allocator->allocate(target->registerNumber);
            // movr %targetReg, %valueReg
            jit->movr(targetReg, valueReg);
        } else if (assign->value->astType == hadron::ast::ASTType::kCalculate) {
            const auto calc = reinterpret_cast<const hadron::ast::CalculateAST*>(assign->value.get());
            // Assumption is it's always a Value on the left and either a Value or Constant on the right,
            // this would happen as part of 3-address tree shaping during Syntax Analysis
            const auto left = reinterpret_cast<const hadron::ast::ValueAST*>(calc->left.get());
            JIT::Reg leftReg = allocator->allocate(left->registerNumber);
            if (calc->selector == hadron::kAddHash) {
                if (calc->right->astType == hadron::ast::ASTType::kConstant) {
                    const auto right = reinterpret_cast<const hadron::ast::ConstantAST*>(calc->right.get());
                    // addi %targetReg, %leftReg, right
                    jit->addi(targetReg, leftReg, right->value.asInteger());
                } else {
                    const auto right = reinterpret_cast<const hadron::ast::ValueAST*>(calc->right.get());
                    JIT::Reg rightReg = allocator->allocate(right->registerNumber);
                    // addr %targetReg, %leftReg, %rightReg
                    jit->addr(targetReg, leftReg, rightReg);
                }
            }
        } else if (assign->value->astType == hadron::ast::ASTType::kConstant) {
            const auto constant = reinterpret_cast<const hadron::ast::ConstantAST*>(assign->value.get());
            // movi %targetReg, constant
            jit->movi(targetReg, constant->value.asInteger());
        }
    } break;

    case hadron::ast::ASTType::kBlock: {
        // TODO for sub-blocks
    } break;

    case hadron::ast::ASTType::kConstant:
    case hadron::ast::ASTType::kValue:
    case hadron::ast::ASTType::kCalculate: {
        // Unexpected at the root level, a sign that something is not right with the 3-address form
    } break;

    case hadron::ast::ASTType::kInlineBlock: {
        // TODO for block inlining
    } break;


default:
break;
    }
}

}  // namespace hadron