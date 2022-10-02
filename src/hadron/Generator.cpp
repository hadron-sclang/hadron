#include "hadron/Generator.hpp"

#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/Function.hpp"
#include "hadron/ThreadContext.hpp"

namespace hadron {

SCMethod Generator::serialize(ThreadContext* context, library::CFGFrame frame) {
    // Compile any inner blocks first.
    for (int32_t i = 0; i < frame.innerBlocks().size(); ++i) {
        auto innerBlock = frame.innerBlocks().typedAt(i);
        SCMethod code = serialize(context, innerBlock.frame());
        auto functionDef = library::FunctionDef::alloc(context);
        functionDef.initToNil();
        functionDef.setCode(Slot::makeRawPointer(reinterpret_cast<int8_t*>(code)));
        functionDef.setSelectors(innerBlock.frame().selectors());
        functionDef.setPrototypeFrame(innerBlock.frame().prototypeFrame());
        innerBlock.setFunctionDef(functionDef);
        frame.setSelectors(frame.selectors().typedAdd(context, functionDef));
    }

    // Map of block number (index) to Block struct, useful when traversing control flow graph.
    std::vector<library::CFGBlock> blocks;
    blocks.resize(frame.numberOfBlocks(), library::CFGBlock());

    auto blockOrder = library::TypedArray<library::BlockId>::typedArrayAlloc(context, frame.numberOfBlocks());

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(context, frame.rootScope().blocks().typedFirst(), blocks, blockOrder);
    blockOrder = blockOrder.typedReverse(context);

    // Build function signature
    assert(!frame.hasVarArgs());
    asmjit::FuncSignatureBuilder signature(asmjit::CallConvId::kHost, 3);
    // Hadron functions always return a slot.
    signature.setRet(asmjit::TypeId::kUInt64);
    // First argument is always the context pointer.
    signature.addArg(asmjit::TypeId::kIntPtr);
    // Second argument is the frame pointer.
    signature.addArg(asmjit::TypeId::kIntPtr);
    // Third argument is the stack pointer.
    signature.addArg(asmjit::TypeId::kIntPtr);

    return buildFunction(context, signature, blocks, blockOrder);
}

// static
bool Generator::markThreadForJITCompilation() {
#if defined(__APPLE__)
    pthread_jit_write_protect_np(false);
#endif
    return true;
}

// static
void Generator::markThreadForJITExecution() {
#if defined(__APPLE__)
    pthread_jit_write_protect_np(true);
#endif
}

// static
schema::FunctionSchema* newFunction(ThreadContext* context) {
    auto f = library::Function::alloc(context);
    return f.instance();
}

void Generator::orderBlocks(ThreadContext* context, library::CFGBlock block, std::vector<library::CFGBlock>& blocks,
                            library::TypedArray<library::BlockId> blockOrder) {
    // Mark block as visited by updating number to pointer map.
    blocks[block.id()] = block;
    for (int32_t i = 0; i < block.successors().size(); ++i) {
        auto succ = block.successors().typedAt(i);
        if (!blocks[succ.id()]) {
            orderBlocks(context, succ, blocks, blockOrder);
        }
    }
    blockOrder = blockOrder.typedAdd(context, static_cast<library::BlockId>(block.id()));
}

} // namespace hadron