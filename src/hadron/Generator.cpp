#include "hadron/Generator.hpp"

#include "asmjit/core/func.h"
#include "asmjit/core/type.h"
#include "hadron/ThreadContext.hpp"

namespace hadron {

SCMethod Generator::serialize(ThreadContext* context, const library::CFGFrame frame) {
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

    return buildFunction(frame, signature, blocks, blockOrder);
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