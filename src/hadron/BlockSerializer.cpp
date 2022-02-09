#include "hadron/BlockSerializer.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/lir/LabelLIR.hpp"
#include "hadron/Scope.hpp"

#include <algorithm>

namespace hadron {

std::unique_ptr<LinearBlock> BlockSerializer::serialize(const Frame* frame) {
    // Prepare the LinearBlock for recording of value lifetimes.
    auto linearBlock = std::make_unique<LinearBlock>();
    linearBlock->blockOrder.reserve(frame->numberOfBlocks);
    linearBlock->blockRanges.resize(frame->numberOfBlocks, std::make_pair(0, 0));
    linearBlock->valueLifetimes.resize(frame->values.size());
    for (size_t i = 0; i < frame->values.size(); ++i) {
        linearBlock->valueLifetimes[i].emplace_back(std::make_unique<LifetimeInterval>());
        linearBlock->valueLifetimes[i][0]->valueNumber = i;
    }

    // Map of block number to Block struct, useful when recursing through control flow graph.
    std::vector<Block*> blockPointers;
    blockPointers.resize(frame->numberOfBlocks, nullptr);

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(frame->rootScope->blocks.front().get(), blockPointers, linearBlock->blockOrder);
    std::reverse(linearBlock->blockOrder.begin(), linearBlock->blockOrder.end());

    // Fill linear block in computed order.
    for (auto blockNumber : linearBlock->blockOrder) {
        auto block = blockPointers[blockNumber];
        auto label = std::make_unique<lir::LabelLIR>(block->number);
        for (auto pred : block->predecessors) {
            label->predecessors.emplace_back(pred->number);
        }
        for (auto succ : block->successors) {
            label->successors.emplace_back(succ->number);
        }
        for (const auto& phi : block->phis) {
            label->phis.emplace_back(phi->lowerPhi(block->frame->values));
        }

        auto blockRange = std::make_pair(linearBlock->instructions.size(), 0);

        // Start the block with a label and then append all contained instructions.
        linearBlock->instructions.emplace_back(std::move(label));
        for (const auto& hir : block->statements) {
            hir->lower(block->frame->values, linearBlock->instructions);
        }

        // Update end range on the block now that it's complete.
        blockRange.second = linearBlock->instructions.size();
        linearBlock->blockRanges[block->number] = std::move(blockRange);
    }

    return linearBlock;
}

void BlockSerializer::orderBlocks(Block* block, std::vector<Block*>& blockPointers, std::vector<int>& blockOrder) {
    // Mark block as visited by updating number to pointer map.
    blockPointers[block->number] = block;
    for (const auto succ : block->successors) {
        if (blockPointers[succ->number] == nullptr) {
            orderBlocks(succ, blockPointers, blockOrder);
        }
    }
    blockOrder.emplace_back(block->number);
}

} // namespace hadron