#ifndef SRC_REGISTER_ALLOCATOR_HPP_
#define SRC_REGISTER_ALLOCATOR_HPP_

#include "hadron/JIT.hpp"

namespace hadron {

namespace ast {
struct BlockAST;
struct ValueAST;
} // namespace ast

// The RegisterAllocator is responsible for allocating register space for ValueAST references within a BlockAST. This
// process may also generate JIT bytecode itself, as registers may need to be *spilled*, that is saved to memory,
// during allocation.
// It is typically used by the CodeGenerator for conversion of the AST to JIT Bytecode, and is provided separately
// mostly for ease of testing.
class RegisterAllocator {
public:
    RegisterAllocator(const ast::BlockAST* block, JIT* jit);
    RegisterAllocator() = delete;
    ~RegisterAllocator() = default;

    // Call after calling jit->prolog(), sets up the stack space required for this Block to spill registers.
    void setupStack();

    // Allocate a physical register for the supplied virtual register.
    JIT::Reg allocate(int vReg);

private:
    const ast::BlockAST* m_block;
    JIT* m_jit;
};

}  // namespace hadron

#endif