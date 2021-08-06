#include "hadron/CodeGenerator.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/VirtualJIT.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "hadron/ThreadContext.hpp"
#include "Keywords.hpp"

#include "fmt/format.h"

#include <string>
#include <set>
#include <vector>

class RegisterAllocator {
public:
    RegisterAllocator(hadron::VirtualJIT* virtualJIT): m_virtualJIT(virtualJIT), m_tempRegisterCount(0) {}
    RegisterAllocator() = delete;
    ~RegisterAllocator() = default;

    struct ScopedRegister {
        ScopedRegister(hadron::JIT::Reg r, ::RegisterAllocator* alloc, bool release): reg(r), allocator(alloc),
            shouldFree(release) {}
        ~ScopedRegister() { free(); }
        void free() {
            if (shouldFree) {
                allocator->freeRegister(reg);
            }
            shouldFree = false;
        }
        hadron::JIT::Reg reg;
        ::RegisterAllocator* allocator;
        bool shouldFree;
    };

    ScopedRegister regForHash(hadron::Hash hash) {
        hadron::JIT::Reg reg = allocateRegister(hash);
        return ScopedRegister(reg, this, true);
    }

    ScopedRegister regForValue(const hadron::ast::ValueAST* value) {
        hadron::JIT::Reg reg = allocateRegister(value->nameHash);
        return ScopedRegister(reg, this, value->canRelease);
    }

    ScopedRegister tempRegister() {
        hadron::JIT::Reg reg = allocateRegister(hadron::hash(fmt::format("_temp_{}", m_tempRegisterCount)));
        m_tempRegisterCount++;
        return ScopedRegister(reg, this, true);
    }

private:
    hadron:: JIT::Reg allocateRegister(hadron::Hash hash) {
        auto allocIter = m_allocatedRegisters.find(hash);
        if (allocIter != m_allocatedRegisters.end()) {
            return allocIter->second;
        }
        hadron::JIT::Reg reg;
        if (m_freeRegisters.size()) {
            reg = *(m_freeRegisters.begin());
            m_freeRegisters.erase(m_freeRegisters.begin());
            m_registerValues[reg] = hash;
        } else {
            reg = m_registerValues.size();
            m_registerValues.emplace_back(hash);
        }
        m_allocatedRegisters.emplace(std::make_pair(hash, reg));
        m_virtualJIT->alias(reg);
        return reg;
    }

    void freeRegister(hadron::JIT::Reg reg) {
        m_allocatedRegisters.erase(m_registerValues[reg]);
        m_freeRegisters.emplace(reg);
        m_virtualJIT->unalias(reg);
    }

    hadron::VirtualJIT* m_virtualJIT;
    std::vector<hadron::Hash> m_registerValues;
    std::unordered_map<hadron::Hash, hadron::JIT::Reg> m_allocatedRegisters;
    std::set<hadron::JIT::Reg> m_freeRegisters;
    size_t m_tempRegisterCount;
};

namespace hadron {

CodeGenerator::CodeGenerator(const ast::BlockAST* block, std::shared_ptr<ErrorReporter> errorReporter):
    m_block(block),
    m_jit(std::make_unique<VirtualJIT>(errorReporter)),
    m_errorReporter(errorReporter) {}

bool CodeGenerator::generate() {
    using ScopedReg = RegisterAllocator::ScopedRegister;
    RegisterAllocator allocator(m_jit.get());

    for (const auto& statement : m_block->statements) {
        jitStatement(statement.get(), &allocator);
    }

    // Load caller return address from framePointer + 1.
    ScopedReg framePointerReg = allocator.tempRegister();
    m_jit->ldxi_w(framePointerReg.reg, JIT::kContextPointerReg, offsetof(ThreadContext, framePointer));
    ScopedReg returnAddressReg = allocator.tempRegister();
    m_jit->ldxi_w(returnAddressReg.reg, framePointerReg.reg, sizeof(Slot) + offsetof(Slot, value.machineCodeAddress));
    m_jit->jmpr(returnAddressReg.reg);
    return true;
}

void CodeGenerator::jitStatement(const ast::AST* ast, RegisterAllocator* allocator) {
    using ScopedReg = RegisterAllocator::ScopedRegister;

    switch (ast->astType) {
    case ast::kCalculate: {
        // wouldn't expect a calculate node right at the root as a statement.
    } break;

    case ast::kBlock: {
        // TODO: block nesting
    } break;

    case ast::kInlineBlock: {
        // TODO: block inlining
    } break;

    case ast::kValue: {
        // not expected at top level
    } break;

    case ast::kSaveToReturnSlot: {
        const auto saveReturn = reinterpret_cast<const ast::SaveToReturnSlotAST*>(ast);
        // Need to load the frame pointer into a register first, it should be directly pointing at the Return
        // value slot.
        ScopedReg framePointerReg = allocator->tempRegister();
        m_jit->ldxi_w(framePointerReg.reg, JIT::kContextPointerReg, offsetof(ThreadContext, framePointer));
        // Save the value into the slot.
        ScopedReg retValueReg = allocator->regForValue(saveReturn->value.get());
        m_jit->stxi_i(offsetof(Slot, value.intValue), framePointerReg.reg, retValueReg.reg);
        retValueReg.free();
        // Load the slot type into a register so it can be written into memory.
        ScopedReg slotTypeReg = allocator->tempRegister();
        m_jit->movi(slotTypeReg.reg, Type::kInteger);
        // Store the type into the Slot memory.
        m_jit->stxi_i(offsetof(Slot, type), framePointerReg.reg, slotTypeReg.reg);
    } break;

    case ast::kAssign: {
        const auto assign = reinterpret_cast<const ast::AssignAST*>(ast);
        ScopedReg targetReg = allocator->regForValue(assign->target.get());
        if (assign->value->astType == ast::ASTType::kValue) {
            const auto assignValue = reinterpret_cast<const ast::ValueAST*>(assign->value.get());
            ScopedReg valueReg = allocator->regForValue(assignValue);
            m_jit->movr(targetReg.reg, valueReg.reg);
        } else if (assign->value->astType == ast::ASTType::kCalculate) {
            const auto calc = reinterpret_cast<const ast::CalculateAST*>(assign->value.get());
            // assumption for now that left is always a value ast
            const auto leftValue = reinterpret_cast<const ast::ValueAST*>(calc->left.get());
            ScopedReg leftReg = allocator->regForValue(leftValue);
            if (calc->right->astType == ast::ASTType::kConstant) {
                const auto rightConst = reinterpret_cast<const ast::ConstantAST*>(calc->right.get());
                if (calc->selector == kAddHash) {
                    m_jit->addi(targetReg.reg, leftReg.reg, rightConst->value.value.intValue);
                }
            } else {
                // assumption that it has to be a value ast
                const auto rightValue = reinterpret_cast<const ast::ValueAST*>(calc->right.get());
                ScopedReg rightReg = allocator->regForValue(rightValue);
                if (calc->selector == kAddHash) {
                    m_jit->addr(targetReg.reg, leftReg.reg, rightReg.reg);
                }
            }
        } else if (assign->value->astType == ast::ASTType::kConstant) {
            const auto constAST = reinterpret_cast<const ast::ConstantAST*>(assign->value.get());
            m_jit->movi(targetReg.reg, constAST->value.value.intValue);
        }
    } break;

    default:
        break;
    }
}

}  // namespace hadron