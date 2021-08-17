#include "hadron/LifetimeAnalyzer.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/SSABuilder.hpp"

namespace hadron {

void LifetimeAnalyzer::buildLifetimes(LinearBlock* linearBlock) {
    // Live range computation iterates through blocks and instructions in reverse order. Now that the prerequisites of
    // the algorithm have been met this is a literal implementation of the pseudocode described in the BuildIntervals
    // algorithm of "Linear Scan Register Allocation on SSA Form" by Christian Wimmer Michael Franz (see Bibliography).
    std::unordered_map<int, std::unordered_set<size_t>> blockLiveIns;

    // for each block b in reverse order do
    for (int i = linearBlock->blockOrder.size() - 1; i >= 0; --i) {
        int blockNumber = linearBlock->blockOrder[i];
        auto blockRange = linearBlock->blockRanges[blockNumber];
        assert(linearBlock->instructions[blockRange.first]->opcode == hir::kLabel);
        const auto blockLabel = reinterpret_cast<const hir::LabelHIR*>(
                linearBlock->instructions[blockRange.first].get());

        // live = union of successor.liveIn for each successor of b
        std::unordered_set<size_t> live;
        for (auto succNumber : blockLabel->successors) {
            const auto& succLiveIn = blockLiveIns[succNumber];
            live.insert(succLiveIn.begin(), succLiveIn.end());

            auto succRange = linearBlock->blockRanges[succNumber];
            assert(linearBlock->instructions[succRange.first]->opcode == hir::kLabel);
            const auto succLabel = reinterpret_cast<const hir::LabelHIR*>(
                    linearBlock->instructions[succRange.first].get());

            // for each phi function phi of successors of b do
            //   live.add(phi.inputOf(b))
            int inputNumber = 0;
            for (auto succPred : succLabel->predecessors) {
                if (succPred == blockNumber) {
                    break;
                }
                ++inputNumber;
            }
            for (const auto& phi : succLabel->phis) {
                live.insert(phi->inputs[inputNumber].number);
            }
        }

        // The next part of the algorithm adds live ranges to the variables used within the block. One operation calls
        // for a modification of a lifetime range (setFrom). Our Lifetime structure doesn't currently support modifying
        // ranges once added, so we save temporary ranges here until final and add them all in then.
        std::unordered_map<int, std::pair<size_t, size_t>> blockVariableRanges;

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
}


} // namespace hadron