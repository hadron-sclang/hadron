#ifndef SRC_REGISTER_ALLOCATOR_HPP_
#define SRC_REGISTER_ALLOCATOR_HPP_

#include "hadron/VirtualJIT.hpp"

namespace hadron {

// SyntaxAnalyzer AST => Code Generator VJIT => RegisterAllocator JIT
class RegisterAllocator {
public:
    RegisterAllocator(VirtualJIT* vJit, JIT* jit);
    RegisterAllocator() = delete;
    ~RegisterAllocator() = default;

private:
};

}  // namespace hadron

#endif