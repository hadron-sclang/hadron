#include "hadron/ThreadContext.hpp"

namespace hadron {

ThreadContext::ThreadContext():
    hadronStack(nullptr),
    stackSize(0),
    framePointer(nullptr),
    exitMachineCode(nullptr),
    machineCodeStatus(0),
    cStackPointer(nullptr) {}

ThreadContext::~ThreadContext() {
    free(hadronStack);
}

bool ThreadContext::allocateStack(size_t size) {
    if (hadronStack) {
        free(hadronStack);
        hadronStack = nullptr;
    }
    hadronStack = malloc(size);
    if (!hadronStack) {
        return false;
    }
    stackSize = size;
    return true;
}



} // namespace hadron