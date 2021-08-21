#include "hadron/BlockSerializer.hpp"

#include "hadron/SSABuilder.hpp"

#include <algorithm>

namespace hadron {

std::unique_ptr<LinearBlock> BlockSerializer::serialize(std::unique_ptr<Frame> baseFrame, size_t numberOfRegisters) {
    // Prepare the LinearBlock for recording of lifetimes in both values and registers.
    auto linearBlock = std::make_unique<LinearBlock>();
    linearBlock->blockOrder.reserve(baseFrame->numberOfBlocks);
    linearBlock->blockRanges.reserve(baseFrame->numberOfBlocks);
    linearBlock->valueLifetimes.resize(baseFrame->numberOfValues, std::vector<LifetimeInterval>(1));
    linearBlock->registerLifetimes.resize(numberOfRegisters, std::vector<LifetimeInterval>(1));
    for (size_t i = 0; i < baseFrame->numberOfValues; ++i) {
        linearBlock->valueLifetimes[i][0].valueNumber = i;
    }
    for (size_t i = 0; i < numberOfRegisters; ++i) {
        linearBlock->registerLifetimes[i][0].registerNumber = i;
    }

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
                reserveRegisters(linearBlock.get());
            }
            linearBlock->instructions.emplace_back(std::move(hir));
            linearBlock->instructions.emplace_back(nullptr);
        }

        blockRange.second = linearBlock->instructions.size() - 1;
        linearBlock->blockRanges[block->number] = std::move(blockRange);
    }

    // Add a block on all physical registers after the last instruction. This simplifies code in the Linear Scan
    // algorithm by ensuring there's always at least one allocation of every register.
    reserveRegisters(linearBlock.get());
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

void BlockSerializer::reserveRegisters(LinearBlock* linearBlock) {
    size_t from = linearBlock->instructions.size();
    size_t to = from + 1;
    for (size_t i = 0; i < linearBlock->registerLifetimes.size(); ++i) {
        linearBlock->registerLifetimes[i][0].addLiveRange(from, to);
        linearBlock->registerLifetimes[i][0].usages.emplace(from);
    }
}

} // namespace hadron