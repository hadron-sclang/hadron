#include "RegisterAllocator.hpp"

#include "hadron/SyntaxAnalyzer.hpp"

namespace hadron {

RegisterAllocator::RegisterAllocator(ast::BlockAST* block, JIT* jit): m_block(block), m_jit(jit) {}

int RegisterAllocator::stackBytesRequired() {
    // TODO: fix me to account for things that aren't either integers or pointers (such as doubles and floats)
    int numberOfIntRegs = m_jit->getRegisterCount(JIT::kSave) + m_jit->getRegisterCount(JIT::kNoSave);
    int overflowSize = std::max(0, static_cast<int>(m_block->variables.size() + m_block->arguments.size()) -
        numberOfIntRegs);
    return overflowSize * sizeof(void*);
}

JIT::Reg RegisterAllocator::allocate(JIT::Reg vReg) {
    return vReg;
}

} // namespace hadron