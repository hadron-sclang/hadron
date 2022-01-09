#include "hadron/BlockSerializer.hpp"

#include "hadron/Block.hpp"
#include "hadron/Frame.hpp"
#include "hadron/LinearBlock.hpp"

#include <algorithm>

namespace hadron {

std::unique_ptr<LinearBlock> BlockSerializer::serialize(std::unique_ptr<Frame> baseFrame) {
    // Prepare the LinearBlock for recording of value lifetimes.
    auto linearBlock = std::make_unique<LinearBlock>();
    linearBlock->blockOrder.reserve(baseFrame->numberOfBlocks);
    linearBlock->blockRanges.resize(baseFrame->numberOfBlocks, std::make_pair(0, 0));
    linearBlock->valueLifetimes.reserve(baseFrame->numberOfValues);
    for (size_t i = 0; i < baseFrame->numberOfValues; ++i) {
        linearBlock->valueLifetimes[i].emplace_back(std::make_unique<LifetimeInterval>());
        linearBlock->valueLifetimes[i][0]->valueNumber = i;
    }

    m_blocks.resize(baseFrame->numberOfBlocks, nullptr);

    // Determine linear block order from reverse postorder traversal.
    orderBlocks(baseFrame->blocks.front().get(), linearBlock->blockOrder);
    std::reverse(linearBlock->blockOrder.begin(), linearBlock->blockOrder.end());

    // Fill linear block in computed order.
    for (auto blockNumber : linearBlock->blockOrder) {
        auto block = m_blocks[blockNumber];
        auto label = std::make_unique<hir::LabelHIR>(block->number);
        for (auto pred : block->predecessors) {
            label->predecessors.emplace_back(pred->number);
        }
        for (auto succ : block->successors) {
            label->successors.emplace_back(succ->number);
        }
        label->phis = std::move(block->phis);

        auto blockRange = std::make_pair(linearBlock->instructions.size(), 0);

        // Start the block with a label and then append all contained instructions.
        linearBlock->instructions.emplace_back(std::move(label));
        std::move(block->statements.begin(), block->statements.end(), std::back_inserter(linearBlock->instructions));

        // Update end range on the block now that it's complete.
        blockRange.second = linearBlock->instructions.size();
        linearBlock->blockRanges[block->number] = std::move(blockRange);
    }

    return linearBlock;
}

void BlockSerializer::orderBlocks(Block* block, std::vector<int>& blockOrder) {
    // Mark block as visited by updating number to pointer map.
    m_blocks[block->number] = block;
    for (const auto succ : block->successors) {
        if (m_blocks[succ->number] != nullptr) {
            orderBlocks(succ, blockOrder);
        }
    }
    blockOrder.emplace_back(block->number);
}

} // namespace hadron