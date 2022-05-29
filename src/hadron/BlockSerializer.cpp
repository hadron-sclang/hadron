#include "hadron/BlockSerializer.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LabelLIR.hpp"

#include <algorithm>

namespace hadron {

std::unique_ptr<LinearFrame> BlockSerializer::serialize(const library::CFGFrame frame) {
    // Prepare the LinearFrame for recording of value lifetimes.
    auto linearFrame = std::make_unique<LinearFrame>();
    linearFrame->blockOrder.reserve(frame.numberOfBlocks());
    linearFrame->blockLabels.resize(frame.numberOfBlocks(), linearFrame->instructions.end());

    // Map of block number to Block struct, useful when recursing through control flow graph.
    std::vector<library::CFGBlock> blocks;
    blocks.resize(frame.numberOfBlocks(), library::CFGBlock());

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(frame.rootScope().blocks().typedFirst(), blocks, linearFrame->blockOrder);
    std::reverse(linearFrame->blockOrder.begin(), linearFrame->blockOrder.end());

    // Fill linear frame with LIR in computed order.
    for (auto blockNumber : linearFrame->blockOrder) {
        auto block = blocks[blockNumber];
        auto label = std::make_unique<lir::LabelLIR>(block.id());
        for (int32_t i = 0; i < block.predecessors().size(); ++i) {
            auto pred = block.predecessors().typedAt(i);
            label->predecessors.emplace_back(pred.id());
        }
        for (int32_t i = 0; i < block.successors().size(); ++i) {
            auto succ = block.successors().typedAt(i);
            label->successors.emplace_back(succ.id());
        }
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

void BlockSerializer::orderBlocks(library::CFGBlock block, std::vector<library::CFGBlock>& blocks,
        std::vector<lir::LabelID>& blockOrder) {
    // Mark block as visited by updating number to pointer map.
    blocks[block.id()] = block;
    for (int32_t i = 0; i < block.successors().size(); ++i) {
        auto succ = block.successors().typedAt(i);
        if (!blocks[succ.id()]) {
            orderBlocks(succ, blocks, blockOrder);
        }
    }
    blockOrder.emplace_back(static_cast<lir::LabelID>(block.id()));
}

} // namespace hadron