#include "hadron/BlockSerializer.hpp"

#include "hadron/library/Array.hpp"
#include "hadron/ThreadContext.hpp"

#include <algorithm>

namespace hadron {

library::LinearFrame BlockSerializer::serialize(ThreadContext* context, const library::CFGFrame frame) {
    // Prepare the LinearFrame for recording of value lifetimes.
    auto linearFrame = library::LinearFrame::alloc(context);
    linearFrame.initToNil();
    linearFrame.setBlockLabels(library::TypedArray<library::LabelLIR>::typedArrayAlloc(context,
            frame.numberOfBlocks()));

    // Map of block number (index) to Block struct, useful when recursing through control flow graph.
    std::vector<library::CFGBlock> blocks;
    blocks.resize(frame.numberOfBlocks(), library::CFGBlock());

    auto blockOrder = library::TypedArray<library::LabelId>::typedArrayAlloc(context, frame.numberOfBlocks());

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(context, frame.rootScope().blocks().typedFirst(), blocks, blockOrder);
    blockOrder = blockOrder.typedReverse(context);
    linearFrame.setBlockOrder(blockOrder);

    // Fill linear frame with LIR in computed order.
    for (int32_t i = 0; i < blockOrder.size(); ++i) {
        auto blockNumber = blockOrder.typedAt(i).int32();
        auto block = blocks[blockNumber];
        auto label = library::LabelLIR::make(context);
        label.setLabelId(block.id());

        auto predecessors = library::TypedArray<library::LabelId>::typedArrayAlloc(context,
                block.predecessors().size());
        for (int32_t i = 0; i < block.predecessors().size(); ++i) {
            auto pred = block.predecessors().typedAt(i);
            predecessors = predecessors.typedAdd(context, pred.id());
        }
        label.setPredecessors(predecessors);

        auto successors = library::TypedArray<library::LabelId>::typedArrayAlloc(context,
                block.successors().size());
        for (int32_t i = 0; i < block.successors().size(); ++i) {
            auto succ = block.successors().typedAt(i);
            successors = successors.typedAdd(context, succ.id());
        }
        label.setSuccessors(successors);
/*
        for (int32_t i = 0; i < block.phis().size(); ++i) {
            auto phi = block.phis().typedAt(i);
            // TODO: this is a duplication of the logic in LinearFrame::append() for phis, and is mixing iterators
            // in vRegs between the label->phi and the linearFrame->instructions which seems likely to create bugs.
            // Reconsider.
            auto phiLIR = phi->lowerPhi(linearFrame.get());
            auto value = static_cast<lir::VReg>(linearFrame->vRegs.size());
            phiLIR->value = value;
            linearFrame->hirToRegMap.emplace(std::make_pair(phi->id, value));
            label->phis.emplace_back(std::move(phiLIR));
            auto iter = label->phis.end();
            --iter;
            linearFrame->vRegs.emplace_back(iter);
        }
        // Start the block with a label and then append all contained instructions.
        linearFrame->instructions.emplace_back(std::move(label));
        linearFrame->blockLabels[block->id()] = --(linearFrame->instructions.end());

        for (const auto& hir : block->statements()) {
            hir->lower(linearFrame.get());
        }
*/
    }
    return linearFrame;
}

void BlockSerializer::orderBlocks(ThreadContext* context, library::CFGBlock block,
        std::vector<library::CFGBlock>& blocks, library::TypedArray<library::LabelId> blockOrder) {
    // Mark block as visited by updating number to pointer map.
    blocks[block.id()] = block;
    for (int32_t i = 0; i < block.successors().size(); ++i) {
        auto succ = block.successors().typedAt(i);
        if (!blocks[succ.id()]) {
            orderBlocks(context, succ, blocks, blockOrder);
        }
    }
    blockOrder = blockOrder.typedAdd(context, static_cast<library::LabelId>(block.id()));
}

} // namespace hadron