#ifndef SRC_HADRON_RUNTIME_HPP_
#define SRC_HADRON_RUNTIME_HPP_

#include <memory>

namespace hadron {

class ClassLibrary;
class ErrorReporter;
class Heap;
struct ThreadContext;

// Owns all of the objects required to compile and run SC code, including the Heap, ThreadContext, and ClassLibrary.
class Runtime {
public:
    Runtime() = delete;
    explicit Runtime(std::shared_ptr<ErrorReporter> errorReporter);
    ~Runtime();

    // Compile (or re-compile) class library.
    bool compileClassLibrary();
    // Finalize members in ThreadContext, initialize language globals needed for the Interpreter. Requires a valid
    // class library compile.
    bool initInterpreter();

    ThreadContext* context() { return m_threadContext.get(); }

private:
    bool buildTrampolines();
    bool buildThreadContext();
    void enterMachineCode(const uint8_t* machineCode);

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::shared_ptr<Heap> m_heap;
    std::unique_ptr<ThreadContext> m_threadContext;
    std::unique_ptr<ClassLibrary> m_classLibrary;

    // Saves registers, initializes thread context and stack pointer registers, and jumps into the machine code pointer.
    void (*m_entryTrampoline)(ThreadContext* context, const uint8_t* machineCode);
    // Restores registers and returns control to C++ code.
    void (*m_exitTrampoline)();
};

} // namespace hadron

#endif // SRC_HADRON_RUNTIME_HPP_