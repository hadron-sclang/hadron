#include "hadron/aarch64/Generator.hpp"

#include "hadron/ThreadContext.hpp"

#include <algorithm>
#include <vector>

namespace hadron {

Generator::Generator() {
    m_codeHolder.init(m_jitRuntime.environment());
}

void Generator::serialize(ThreadContext* context, const library::CFGFrame frame) {
    // Create compiler object and attach to code space
    asmjit::a64::Compiler compiler(&m_codeHolder);

    // Build function signature
    // TODO: varArgs
    assert(!frame.hasVarArgs());
    asmjit::FuncSignatureBuilder signature;
    // Hadron functions always return a slot.
    signature.setRet(asmjit::TypeId::kUInt64);
    // First argument is always the context pointer.
    signature.addArg(asmjit::TypeId::kUIntPtr);
    // All remaining arguments are Slots
    for (int32_t i = 0; i < frame.argumentNames().size(); ++i) {
        signature.addArg(asmjit::TypeId::kUInt64);
    }

    auto funcNode = compiler.addFunc(signature);

    // Map of block number (index) to Block struct, useful when traversing control flow graph.
    std::vector<library::CFGBlock> blocks;
    blocks.resize(frame.numberOfBlocks(), library::CFGBlock());

    auto blockOrder = library::TypedArray<library::BlockId>::typedArrayAlloc(context, frame.numberOfBlocks());

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(context, frame.rootScope().blocks().typedFirst(), blocks, blockOrder);
    blockOrder = blockOrder.typedReverse(context);

    std::vector<asmjit::Label> blockLabels(blocks.size());
    // TODO: maybe lazily create, as some blocks may have been deleted?
    std::generate(blockLabels.begin(), blockLabels.end(), [&compiler]() { return compiler.newLabel(); });

    for (int32_t i = 0; i < blockOrder.size(); ++i) {
        auto blockNumber = blockOrder.typedAt(i).int32();
        auto block = blocks[blockNumber];

        // Bind label to current position in the code.
        compiler.bind(blockLabels[blockNumber]);

        // Phis can be resolved with creation of new virtual reg associated with phi followed by a jump to the
        // next HIR instruction

    }
}

void Generator::orderBlocks(ThreadContext* context, library::CFGBlock block,
                                  std::vector<library::CFGBlock>& blocks,
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