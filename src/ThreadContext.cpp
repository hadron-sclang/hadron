#include "hadron/ThreadContext.hpp"

namespace hadron {

ThreadContext::ThreadContext():
    stackSize(0),
    hadronStack(nullptr),
    framePointer(nullptr),
    stackPointer(nullptr),
    exitMachineCode(nullptr),
    machineCodeStatus(0),
    cStackPointer(nullptr) {}

ThreadContext::~ThreadContext() {
    free(hadronStack);
}

bool ThreadContext::allocateStack(size_t size) {
    // TODO: this should probably move over to mmap()
    if (hadronStack) {
        free(hadronStack);
        hadronStack = nullptr;
    }
    hadronStack = reinterpret_cast<Slot*>(malloc(size * sizeof(Slot)))
    if (!hadronStack) {
        stackSize = 0;
        framePointer = nullptr;
        stackPointer = nullptr;
        return false;
    }

    stackSize = size;
    framePointer = hadronStack + stackSize - 1;
    stackPointer = framePointer;
    return true;
}



} // namespace hadron