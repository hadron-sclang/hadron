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