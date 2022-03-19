#include "hadron/BlockSerializer.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LabelLIR.hpp"
#include "hadron/Scope.hpp"

#include <algorithm>

namespace hadron {

std::unique_ptr<LinearFrame> BlockSerializer::serialize(const Frame* frame) {
    // Prepare the LinearFrame for recording of value lifetimes.
    auto linearFrame = std::make_unique<LinearFrame>();
    linearFrame->blockOrder.reserve(frame->numberOfBlocks);
    linearFrame->blockLabels.resize(frame->numberOfBlocks, linearFrame->instructions.end());
    // Reserve room for all the values in the HIR, so new values are appended after.
    linearFrame->vRegs.resize(frame->values.size(), linearFrame->instructions.end());

    // Map of block number to Block struct, useful when recursing through control flow graph.
    std::vector<Block*> blockPointers;
    blockPointers.resize(frame->numberOfBlocks, nullptr);

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(frame->rootScope->blocks.front().get(), blockPointers, linearFrame->blockOrder);
    std::reverse(linearFrame->blockOrder.begin(), linearFrame->blockOrder.end());

    // Fill linear frame with LIR in computed order.
    for (auto blockNumber : linearFrame->blockOrder) {
        auto block = blockPointers[blockNumber];
        auto label = std::make_unique<lir::LabelLIR>(block->id());
        for (auto pred : block->predecessors()) {
            label->predecessors.emplace_back(pred->id());
        }
        for (auto succ : block->successors()) {
            label->successors.emplace_back(succ->id());
        }
        for (const auto& phi : block->phis()) {
            phi->lower(block->frame()->values, linearFrame->vRegs, label->phis);
        }
        // Start the block with a label and then append all contained instructions.
        linearFrame->instructions.emplace_back(std::move(label));
        linearFrame->blockLabels[block->id()] = --(linearFrame->instructions.end());

        for (const auto& hir : block->statements()) {
            hir->lower(block->frame()->values, linearFrame->vRegs, linearFrame->instructions);
        }
    }

    return linearFrame;
}

void BlockSerializer::orderBlocks(Block* block, std::vector<Block*>& blockPointers,
        std::vector<lir::LabelID>& blockOrder) {
    // Mark block as visited by updating number to pointer map.
    blockPointers[block->id()] = block;
    for (const auto succ : block->successors()) {
        if (blockPointers[succ->id()] == nullptr) {
            orderBlocks(succ, blockPointers, blockOrder);
        }
    }
    blockOrder.emplace_back(static_cast<lir::LabelID>(block->id()));
}

} // namespace hadron