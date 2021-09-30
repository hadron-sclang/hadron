#ifndef SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_

#include "Slot.hpp"

#include <cstddef>
#include <cstdint>

namespace hadron {

struct ThreadContext {
    ThreadContext();
    ~ThreadContext();

    static constexpr size_t kDefaultStackSize = 1024 * 1024;
    bool allocateStack(size_t size = kDefaultStackSize);

    // We keep a separate stack for Hadron JIT from the main C/C++ application stack.
    size_t stackSize;
    Slot* hadronStack;
    Slot* framePointer;
    Slot* stackPointer;

    // The return address to restore the C stack and exit the machine code ABI.
    const uint8_t* exitMachineCode;
    // An exit flag that can be set to indicate unusual exit conditions.
    int machineCodeStatus;

    // The stack pointer as preserved on entry into machine code.
    void* cStackPointer;
};

} // namespace hadron

#endif // SRC_COMPILER_HADRON_INCLUDE_THREAD_CONTEXT_HPP_