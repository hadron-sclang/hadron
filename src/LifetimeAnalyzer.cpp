#include "hadron/LifetimeAnalyzer.hpp"

#include "hadron/SSABuilder.hpp"

#include <algorithm>

namespace hadron {

LifetimeAnalyzer::LifetimeAnalyzer() {}

LifetimeAnalyzer::~LifetimeAnalyzer() {}

std::unique_ptr<LinearBlock> LifetimeAnalyzer::buildLifetimes(std::unique_ptr<Frame> baseFrame) {
    auto linearBlock = std::make_unique<LinearBlock>();
    // To simplify counting with unsigned values insert an empty instruction at the start of the linear block.
    linearBlock->instructions.emplace_back(nullptr);

    // Determine linear block order from reverse postorder traversal.
    std::unordered_map<int, Block*> blockMap;
    orderBlocks(baseFrame->blocks.front().get(), linearBlock->blockOrder, blockMap);
    std::reverse(linearBlock->blockOrder.begin(), linearBlock->blockOrder.end());

    // Fill linear block in computed order.
    for (auto blockNumber : linearBlock->blockOrder) {
        auto block = blockMap[blockNumber];
        auto label = std::make_unique<hir::LabelHIR>(block->number);
        label->phis = std::move(block->phis);
        auto blockRange = std::make_pair(linearBlock->instructions.size(), 0);

        // Start the block with a label and then append all contained instructions.
        linearBlock->instructions.emplace_back(std::move(label));
        linearBlock->instructions.emplace_back(nullptr);
        for (auto& hir : block->statements) {
            linearBlock->instructions.emplace_back(std::move(hir));
            linearBlock->instructions.emplace_back(nullptr);
        }

        blockRange.second = linearBlock->instructions.size() - 1;
        linearBlock->blockRanges.emplace(std::make_pair(block->number, blockRange));
    }

    std::unordered_map<int, std::unordered_set<int>> blockLiveIns;

    // Live range computation iterates through blocks and instructions in reverse order. Now that the prerequisites of
    // the algorithm have been met this is a literal implementation of the pseudocode described in the BuildIntervals
    // algorithm of "Linear Scan Register Allocation on SSA Form" by Christian Wimmer Michael Franz (see Bibliography).

    // for each block b in reverse order do
    for (int i = linearBlock->blockOrder.size() - 1; i >= 0; --i) {
        int blockNumber = linearBlock->blockOrder[i];
        auto block = blockMap[blockNumber];

        // live = union of successor.liveIn for each successor of b
        std::unordered_set<int> live;
        for (auto succ : block->successors) {
            const auto& succLiveIn = blockLiveIns[succ->number];
            live.insert(succLiveIn.begin(), succLiveIn.end());

            // for each phi function phi of successors of b do
            //   live.add(phi.inputOf(b))
            int inputNumber = 0;
            for (auto succPred : succ->predecessors) {
                if (succPred == block) {
                    break;
                }
                ++inputNumber;
            }
            auto succRange = linearBlock->blockRanges[succ->number];
            assert(linearBlock->instructions[succRange.first]->opcode == hir::kLabel);
            const auto succLabel = reinterpret_cast<const hir::LabelHIR*>(
                    linearBlock->instructions[succRange.first].get());
            for (const auto& phi : succLabel->phis) {
                live.insert(phi->inputs[inputNumber].number);
            }
        }

        // for each opd in live do
        //   intervals[opd].addRange(b.from, b.to)

        // auto blockRange = linearBlock->blockRanges[blockNumber];
    }

    return linearBlock;
}

void LifetimeAnalyzer::orderBlocks(Block* block, std::vector<int>& blockOrder,
        std::unordered_map<int, Block*>& blockMap) {
    // Mark block as visited by updating number to pointer map.
    blockMap.emplace(std::make_pair(block->number, block));
    for (const auto succ : block->successors) {
        if (blockMap.find(succ->number) == blockMap.end()) {
            orderBlocks(succ, blockOrder, blockMap);
        }
    }
    blockOrder.emplace_back(block->number);
}

} // namespace hadron