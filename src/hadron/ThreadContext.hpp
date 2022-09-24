#ifndef SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_THREAD_CONTEXT_HPP_

#include "hadron/Slot.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace hadron {
namespace schema {
struct ArraySchema;
struct FramePrivateSchema;
struct ProcessSchema;
struct ThreadSchema;
} // namespace schema

class ClassLibrary;
class Generator;
class Heap;
class SymbolTable;

struct ThreadContext {
    ThreadContext() = default;
    ~ThreadContext() = default;

    // We keep a separate stack for Hadron JIT from the main C/C++ application stack.
    schema::FramePrivateSchema* framePointer = nullptr;
    schema::FramePrivateSchema* stackPointer = nullptr;

    enum InterruptCode : int32_t { kDispatch, kFatalError, kNewObject, kPrimitive };
    InterruptCode interruptCode = InterruptCode::kFatalError;

    std::shared_ptr<Heap> heap;
    std::unique_ptr<SymbolTable> symbolTable;
    std::unique_ptr<ClassLibrary> classLibrary;
    std::unique_ptr<Generator> generator;

    // Objects accessible from the language. To break the cyclical dependency between library objects and ThreadContext,
    // but still keep strongly typed references here, we maintain forward-decleared instance pointers, and then just
    // always wrap them in their corresponding library objects when using them from the C++ side.
    schema::ProcessSchema* thisProcess;
    schema::ThreadSchema* thisThread;
    schema::ArraySchema* classVariablesArray;
};

// ThreadContext is accessed by machine code, so needs a simple layout in memory.
static_assert(std::is_standard_layout<ThreadContext>::value);

} // namespace hadron

#endif // SRC_COMPILER_HADRON_INCLUDE_THREAD_CONTEXT_HPP_