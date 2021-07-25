#ifndef SRC_HADRON_INCLUDE_THREAD_CONTEXT_HPP_
#define SRC_HADRON_INCLUDE_THREAD_CONTEXT_HPP_

namespace hadron {

struct ThreadContext {
    ThreadContext();
    ~ThreadContext();

    constexpr size_t kDefaultStackSize = 1024 * 1024;
    bool allocateStack(size_t size = kDefaultStackSize);

    // We keep a separate stack for Hadron JIT from the main C/C++ application stack.
    void* hadronStack;
    size_t stackSize;
    void* framePointer;

    // The return address to restore the C stack and exit the machine code ABI.
    void* exitMachineCode;
    int machineCodeStatus;
    // The stack pointer as preserved on entry into machine code.
    void* cStackPointer;
};

} // namespace hadron

#endif // SRC_HADRON_INCLUDE_THREAD_CONTEXT_HPP_