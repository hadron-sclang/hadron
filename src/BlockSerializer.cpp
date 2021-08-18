#include "hadron/BlockSerializer.hpp"

#include "hadron/SSABuilder.hpp"

#include <algorithm>

namespace hadron {

std::unique_ptr<LinearBlock> BlockSerializer::serialize(std::unique_ptr<Frame> baseFrame, size_t numberOfRegisters) {
    auto linearBlock = std::make_unique<LinearBlock>(baseFrame->numberOfBlocks, baseFrame->numberOfValues,
            numberOfRegisters);
    m_blocks.resize(baseFrame->numberOfBlocks, nullptr);

    // To simplify counting with unsigned values insert an empty instruction at the start of the linear block.
    linearBlock->instructions.emplace_back(nullptr);

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
        linearBlock->instructions.emplace_back(nullptr);
        for (auto& hir : block->statements) {
            // Mark all registers as in-use for any dispatch, to later force the register allocator to save all active
            // register values.
            if (hir->opcode == hir::kDispatchCall) {
                size_t lineNumber = linearBlock->instructions.size();
                for (auto& lifetime : linearBlock->registerLifetimes) {
                    lifetime.addInterval(lineNumber, lineNumber + 1);
                    lifetime.usages.emplace(lineNumber);
                }
            }
            linearBlock->instructions.emplace_back(std::move(hir));
            linearBlock->instructions.emplace_back(nullptr);
        }

        blockRange.second = linearBlock->instructions.size() - 1;
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