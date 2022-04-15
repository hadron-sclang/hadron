#ifndef SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_

#include "hadron/Slot.hpp"

#include <cstddef>
#include <cstdint>

namespace hadron {
namespace schema {
struct ProcessSchema;
struct ThreadSchema;
}

class ClassLibrary;
class Heap;
class SymbolTable;

struct ThreadContext {
    ThreadContext() = default;
    ~ThreadContext() = default;

    // We keep a separate stack for Hadron JIT from the main C/C++ application stack.
    size_t stackSize = 0;
    int8_t* hadronStack = nullptr;
    int8_t* framePointer = nullptr;
    int8_t* stackPointer = nullptr;

    // The return address to restore the C stack and exit the machine code ABI.
    const int8_t* exitMachineCode = nullptr;

    enum InterruptCode : int32_t {
        kNormalReturn,
        kDispatch,
        kAllocateMemory
    };
    InterruptCode interruptCode = InterruptCode::kNormalReturn;

    // The stack pointer as preserved on entry into machine code.
    void* cStackPointer = nullptr;

    std::shared_ptr<Heap> heap;
    std::unique_ptr<SymbolTable> symbolTable;
    std::unique_ptr<ClassLibrary> classLibrary;

    // Objects accessible from the language. To break the cyclical dependency between library objects and ThreadContext,
    // but still keep strongly typed references here, we maintain forward-decleared instance pointers, and then just
    // always wrap them in their corresponding library objects when using them from the C++ side.
    schema::ProcessSchema* thisProcess;
    schema::ThreadSchema* thisThread;

    // If true, Hadron will run internal diagnostics during every compilation step.
    bool runInternalDiagnostics = true;
};

} // namespace hadron

#endif // SRC_COMPILER_HADRON_INCLUDE_THREAD_CONTEXT_HPP_