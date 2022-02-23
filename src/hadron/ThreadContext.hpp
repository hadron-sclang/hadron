#ifndef SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_

#include "hadron/Slot.hpp"

#include <cstddef>
#include <cstdint>

namespace hadron {
namespace schema {
struct ProcessSchema;
}

class Heap;
class SymbolTable;

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

    std::shared_ptr<Heap> heap;
    std::unique_ptr<SymbolTable> symbolTable;

    // Objects accessible from the language. To break the cyclical dependency between library objects and ThreadContext,
    // but still keep strongly typed references here, we maintain forward-decleared instance pointers, and then just
    // always wrap them in their corresponding library objects when using them from the C++ side.
    schema::ProcessSchema* thisProcess;

    // If true, Hadron will run internal diagnostics during every compilation step.
    bool runInternalDiagnostics = true;
};

} // namespace hadron

#endif // SRC_COMPILER_HADRON_INCLUDE_THREAD_CONTEXT_HPP_