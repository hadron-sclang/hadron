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

    // Live range computation iterates through blocks and instructions in reverse order. Now that the prerequisites of
    // the algorithm have been met this is a literal implementation of the pseudocode described in the BuildIntervals
    // algorithm of "Linear Scan Register Allocation on SSA Form" by Christian Wimmer Michael Franz (see Bibliography).
    std::unordered_map<int, std::unordered_set<size_t>> blockLiveIns;

    // for each block b in reverse order do
    for (int i = linearBlock->blockOrder.size() - 1; i >= 0; --i) {
        int blockNumber = linearBlock->blockOrder[i];
        auto block = blockMap[blockNumber];

        // live = union of successor.liveIn for each successor of b
        std::unordered_set<size_t> live;
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

        // The next part of the algorithm adds live ranges to the variables used within the block. One operation calls
        // for a modification of a lifetime range (setFrom). Our Lifetime structure doesn't currently support modifying
        // ranges once added, so we save temporary ranges here until final and add them all in then.
        std::unordered_map<int, std::pair<size_t, size_t>> blockVariableRanges;
        auto blockRange = linearBlock->blockRanges[blockNumber];

        // for each opd in live do
        //   intervals[opd].addRange(b.from, b.to)
        for (auto opd : live) {
            blockVariableRanges[opd] = blockRange;
        }

        // for each operation op of b in reverse order do
        for (size_t j = blockRange.second; j >= blockRange.first; --j) {
            const hir::HIR* hir = linearBlock->instructions[j].get();
            if (!hir) {
                continue;
            }

            // for each output operand opd of op do
            //   intervals[opd].setFrom(op.id)
            blockVariableRanges[hir->value.number] = std::make_pair(j, blockRange.second);
            //   live.remove(opd)
            live.erase(hir->value.number);

            // for each input operand opd of op do
            for (auto opd : hir->reads) {
                // intervals[opd].addRange(b.from, op.id)
                blockVariableRanges[opd.number] = std::make_pair(blockRange.first, j + 1);
                // live.add(opd)
                live.insert(opd.number);
            }
        }

        assert(linearBlock->instructions[blockRange.first]->opcode == hir::kLabel);
        const auto blockLabel = reinterpret_cast<const hir::LabelHIR*>(
                linearBlock->instructions[blockRange.first].get());

        // for each phi function phi of b do
        for (const auto& phi : blockLabel->phis) {
            // live.remove(phi.output)
            live.erase(phi->value.number);
        }

        // TODO: loop header step
        // if b is loop header then
        //   loopEnd = last block of the loop starting at b
        //   for each opd in live do
        //     intervals[opd].addRange(b.from, loopEnd.to)

        // b.liveIn = live
        blockLiveIns.emplace(std::make_pair(blockNumber, std::move(live)));

        // Cleanup step, add the (now final) ranges into the lifetimes.
        for (auto rangePair : blockVariableRanges) {
            auto lifetimeIter = linearBlock->lifetimes.emplace(rangePair.first, Lifetime()).first;
            lifetimeIter->second.addInterval(rangePair.second.first, rangePair.second.second);
        }
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