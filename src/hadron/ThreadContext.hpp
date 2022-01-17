#ifndef SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_

#include "hadron/Slot.hpp"

#include <cstddef>
#include <cstdint>

namespace hadron {

class Heap;

struct ThreadContext {
    ThreadContext() = default;
    ~ThreadContext() = default;

    // We keep a separate stack for Hadron JIT from the main C/C++ application stack.
    size_t stackSize = 0;
    Slot* hadronStack = nullptr;
    Slot* framePointer = nullptr;
    Slot* stackPointer = nullptr;

    // The return address to restore the C stack and exit the machine code ABI.
    const uint8_t* exitMachineCode = nullptr;
    // An exit flag that can be set to indicate unusual exit conditions.
    int machineCodeStatus = 0;

    // The stack pointer as preserved on entry into machine code.
    void* cStackPointer = nullptr;

    // Objects accessible from the language.
    Slot thisProcess = Slot::makeNil();

    std::shared_ptr<Heap> heap;
};

} // namespace hadron

#endif // SRC_COMPILER_HADRON_INCLUDE_THREAD_CONTEXT_HPP_