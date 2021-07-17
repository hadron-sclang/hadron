#include "hadron/CodeGenerator.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/VirtualJIT.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SyntaxAnalyzer.hpp"
#include "Keywords.hpp"

#include "fmt/format.h"

#include <string>
#include <set>
#include <vector>

class RegisterAllocator {
public:
    RegisterAllocator(hadron::VirtualJIT* virtualJIT): m_virtualJIT(virtualJIT) {}
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
};

namespace hadron {

CodeGenerator::CodeGenerator(const ast::BlockAST* block, std::shared_ptr<ErrorReporter> errorReporter):
    m_block(block),
    m_jit(std::make_unique<VirtualJIT>(errorReporter)),
    m_errorReporter(errorReporter) {}

bool CodeGenerator::generate() {
    m_jit->prolog();
    // We always call allocai as that gives the MachineCodeGenerator a chance to add any stack space needed for
    // register spilling. In the future there may be stack variables so this number can be nonzero as well, and I guess
    // the register spilling stack would exist above this stack space, to keep the addresses here valid.
    m_jit->allocai(0);
    // As this is a top-level function we want it to be c-callable, so set up the stack frame as expected.
//    m_jit->frame(0);

    // First argument is always the Slot return address. ** For now all arguments can be assumed to be addresses.
    // Slot variables can also live on the stack, and will have addresses relative to the frame pointer.
    m_addresses.emplace(std::make_pair(hash("_blockValue"), m_jit->arg()));
    RegisterAllocator allocator(m_jit.get());

    for (const auto& statement : m_block->statements) {
        jitStatement(statement.get(), &allocator);
    }

    // Any non-zero return integer value means function success.
    m_jit->reti(1);
    m_jit->epilog();
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

    // TODO: can the syntax analysis part get the tree into rough 3-addr form?
    // TODO: Possible RAII form for Register allocation?

    case ast::kSaveToSlot: {
        const auto saveToSlot = reinterpret_cast<const ast::SaveToSlotAST*>(ast);
        // Load the address of the slot into a register.
        ScopedReg slotAddressReg = allocator->regForHash(hash(fmt::format("_addr_{:016x}",
            saveToSlot->value->nameHash)));
        JIT::Label addressValue = m_addresses.find(saveToSlot->value->nameHash)->second;
        m_jit->getarg_w(slotAddressReg.reg, addressValue);
        // Store the integer value into the Slot memory. TODO: this register should already be allocated, and it not
        // being so is an error condition.
        ScopedReg slotValueReg = allocator->regForValue(saveToSlot->value.get());
        if (offsetof(Slot, intValue)) {
            m_jit->stxi_i(offsetof(Slot, intValue), slotAddressReg.reg, slotValueReg.reg);
        } else {
            m_jit->str_i(slotAddressReg.reg, slotValueReg.reg);
        }
        slotValueReg.free();
        // Load the slot type into a register so it can be written into memory.
        ScopedReg slotTypeReg = allocator->regForHash(hash(fmt::format("_type_{:08x}", saveToSlot->value->valueType)));
        m_jit->movi(slotTypeReg.reg, Type::kInteger);
        // Store the type into the Slot memory.
        if (offsetof(Slot, type)) {
            m_jit->stxi_i(offsetof(Slot, type), slotAddressReg.reg, slotTypeReg.reg);
        } else {
            m_jit->str_i(slotAddressReg.reg, slotTypeReg.reg);
        }
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
                    m_jit->addi(targetReg.reg, leftReg.reg, rightConst->value.asInteger());
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
            m_jit->movi(targetReg.reg, constAST->value.asInteger());
        }
    } break;

    default:
        break;
    }
}

}  // namespace hadron