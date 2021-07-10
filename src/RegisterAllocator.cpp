#include "RegisterAllocator.hpp"

#include "hadron/SyntaxAnalyzer.hpp"

namespace hadron {

RegisterAllocator::RegisterAllocator(const ast::BlockAST* block, JIT* jit): m_block(block), m_jit(jit) {}

void RegisterAllocator::setupStack() {
    int totalRegisters = m_jit->getRegisterCount(JIT::kNoSave) + m_jit->getRegisterCount(JIT::kSave);
    int spillRegistersNeeded = std::max(0, m_block->numberOfVirtualRegisters - totalRegisters);
    int floatSpillRegistersNeeded = std::max(0,
        m_block->numberOfVirtualFloatRegisters - m_jit->getRegisterCount(JIT::kFloat));
    int stackBytesNeeded = (spillRegistersNeeded * sizeof(void*)) + (floatSpillRegistersNeeded * sizeof(double));
    if (stackBytesNeeded) {
        m_jit->allocai(stackBytesNeeded);
    }
}

JIT::Reg RegisterAllocator::allocate(int vReg) {
    return JIT::Reg(JIT::kSave, vReg);
}

} // namespace hadron