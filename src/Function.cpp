#include "hadron/Function.hpp"

#include "hadron/LighteningJIT.hpp"
#include "hadron/SyntaxAnalyzer.hpp"

namespace hadron {

Function::Function(const ast::BlockAST* block): hadronEntry(nullptr), cWrapper(nullptr) {
    numberOfArgs = blockAST->arguments.size();
    if (numberOfArgs) {
        argumentNames = std::make_unique<Hash[]>(numberOfArgs);
        // we don't know their order right now from the BlockAST
        defaultValues = std::make_unique<Slot[]>(numberOfArgs);
        // since we don't know order we don't know values
        nameIndices.resize(numberOfArgs);
        // build map from names back to indices
    }
}

bool Function::buildEntryTrampoline(LighteningJIT* jit) {
    auto stackSize = jit->enterABI();
    // On function entry, JITted code expects a pointer to ThreadContext in GPR0, and the arguments to the function
    // in order on the stack starting with argument 0 at SP + sizeof(void*) for the calling address. Each Slot is
    // 16 bytes, so stack looks like: (higher values of stack address at top)

    // ARG N-1
    // ARG N-2                       SP + WordSize + ((N -2) * 16)
    // ....
    // ARG 0                         SP + WordSize
    // Return Value Slot             SP + 16 - will be at the top of the stack after return
    // Caller Return Address ======= SP + 0
    // Virtual Reg Spill 0           SP - WordSize
    // Virtual Reg Spill 1           SP - (WordSize * 2)

    // On the caller side, there will be a count and list of ordered args, then a count and list of keyword/arg pairs.
    // Calling from within Hadron ABI, there's a part where we do dispatch using target and selector hashes to find
    // this function object, then we have the same entry conditions as here, which is that there are the two lists
    // of arguments and their counts available somewhere.

    jit->leaveABI(stackSize);
}

Slot Function::value(int numOrderedArgs, Slot* orderedArgs, int numKeywordArgs, Slot* keywordArgs) {
    return Slot();
}

} // namespace hadron