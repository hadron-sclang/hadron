#include "hadron/LifetimeAnalyzer.hpp"

#include "hadron/SSABuilder.hpp"

#include <algorithm>

namespace hadron {

LifetimeAnalyzer::LifetimeAnalyzer() {}

LifetimeAnalyzer::~LifetimeAnalyzer() {}

std::unique_ptr<LinearBlock> LifetimeAnalyzer::buildLifetimes(std::unique_ptr<Frame> baseFrame) {
    // Determine linear block order from reverse postorder traversal.
    orderBlocks(baseFrame->blocks.front().get());
    std::reverse(m_blockOrder.begin(), m_blockOrder.end());

    // Fill linear block in computed order.
    auto linearBlock = std::make_unique<LinearBlock>();
    for (auto blockNumber : m_blockOrder) {
        auto block = m_blocks.find(blockNumber)->second;
        
    }

    return linearBlock;
}

void LifetimeAnalyzer::orderBlocks(Block* block) {
    m_blocks.emplace(std::make_pair(block->number, block));
    for (const auto succ : block->successors) {
        if (m_blocks.find(succ->number) == m_blocks.end()) {
            orderBlocks(succ);
        }
    }
    m_blockOrder.emplace_back(block->number);
}

} // namespace hadron