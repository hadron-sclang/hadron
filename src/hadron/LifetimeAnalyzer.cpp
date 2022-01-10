#include "hadron/LifetimeAnalyzer.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/LinearBlock.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

/*
Pseudocode for the lifetime interval building algorithm taken verbatiim from [RA5] in the Bibliography,  "Linear Scan
Register Allocation on SSA Form" by C. Wimmer and M. Franz.

BUILDINTERVALS
    for each block b in reverse order do
        live = union of successor.liveIn for each successor of b

        for each phi function phi of successors of b do
            live.add(phi.inputOf(b))

        for each opd in live do
            intervals[opd].addRange(b.from, b.to)

        for each operation op of b in reverse order do
            for each output operand opd of op do
                intervals[opd].setFrom(op.id)
                live.remove(opd)
            for each input operand opd of op do
                intervals[opd].addRange(b.from, op.id)
                live.add(opd)

        for each phi function phi of b do
            live.remove(phi.output)

        if b is loop header then
            loopEnd = last block of the loop starting at b
            for each opd in live do
            intervals[opd].addRange(b.from, loopEnd.to)

        b.liveIn = live
*/

void LifetimeAnalyzer::buildLifetimes(LinearBlock* linearBlock) {
    // for each block b in reverse order do
    for (int i = linearBlock->blockOrder.size() - 1; i >= 0; --i) {
        int blockNumber = linearBlock->blockOrder[i];
        auto blockRange = linearBlock->blockRanges[blockNumber];
        assert(linearBlock->instructions[blockRange.first]->opcode == hir::kLabel);
        auto blockLabel = reinterpret_cast<hir::LabelHIR*>(linearBlock->instructions[blockRange.first].get());

        // live = union of successor.liveIn for each successor of b
        std::unordered_set<size_t> live;
        for (auto succNumber : blockLabel->successors) {
            auto succRange = linearBlock->blockRanges[succNumber];
            assert(linearBlock->instructions[succRange.first]->opcode == hir::kLabel);
            const auto succLabel = reinterpret_cast<const hir::LabelHIR*>(
                    linearBlock->instructions[succRange.first].get());

            live.insert(succLabel->liveIns.begin(), succLabel->liveIns.end());

            int inputNumber = 0;
            for (auto succPred : succLabel->predecessors) {
                if (succPred == blockNumber) {
                    break;
                }
                ++inputNumber;
            }
            // for each phi function phi of successors of b do
            //   live.add(phi.inputOf(b))
            for (const auto& phi : succLabel->phis) {
                live.insert(phi->inputs[inputNumber].number);
            }
        }

        // The next part of the algorithm adds live ranges to the variables used within the block. One operation calls
        // for a modification of a lifetime range (setFrom). Our Lifetime structure doesn't currently support modifying
        // ranges once added, so we save temporary ranges here until final and add them all in then.
        std::vector<std::pair<size_t, size_t>> blockVariableRanges(linearBlock->valueLifetimes.size(),
                std::make_pair(std::numeric_limits<size_t>::max(), 0));

        // for each opd in live do
        for (auto opd : live) {
            // intervals[opd].addRange(b.from, b.to)
            blockVariableRanges[opd] = blockRange;
        }

        // for each operation op of b in reverse order do
        for (size_t j = blockRange.second - 1; j >= blockRange.first; --j) {
            const hir::HIR* hir = linearBlock->instructions[j].get();
            // In Hadron there's at most 1 valid output from an HIR so this for loop is instead an if statement.
            // for each output operand opd of op do
            if (hir->value.isValid()) {
                // intervals[opd].setFrom(op.id)
                blockVariableRanges[hir->value.number].first = j;
                linearBlock->valueLifetimes[hir->value.number][0]->usages.emplace(j);

                // live.remove(opd)
                live.erase(hir->value.number);
            }

            // for each input operand opd of op do
            for (auto opd : hir->reads) {
                // intervals[opd].addRange(b.from, op.id)
                blockVariableRanges[opd.number].first = blockRange.first;
                blockVariableRanges[opd.number].second = std::max(j + 1, blockVariableRanges[opd.number].second);
                linearBlock->valueLifetimes[opd.number][0]->usages.emplace(j);
                // live.add(opd)
                live.insert(opd.number);
            }

            // Avoid unsigned comparison causing infinite loops with >= 0.
            if (j == 0) { break; }
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
        blockLabel->liveIns.swap(live);

        SPDLOG_DEBUG("LifetimeAnalyzer Block Ranges");

        // Cleanup step, add the now final ranges into the lifetimes.
        for (size_t j = 0; j < blockVariableRanges.size(); ++j) {
            SPDLOG_DEBUG("** value: {} start: {} end: {}", j, blockVariableRanges[j].first,
                    blockVariableRanges[j].second);
            if (blockVariableRanges[j].first != std::numeric_limits<size_t>::max()) {
                // It's possible a value is created in this block but not read. This could be a sign of code that needs
                // more optimization, or a value that's needed in a subsequent block. Record this as a range only around
                // the usage.
                if (blockVariableRanges[j].second == 0) {
                    blockVariableRanges[j].second = blockVariableRanges[j].first + 1;
                }
                assert(blockVariableRanges[j].second > blockVariableRanges[j].first);
                linearBlock->valueLifetimes[j][0]->addLiveRange(blockVariableRanges[j].first,
                    blockVariableRanges[j].second);
            }
        }
    }
}

} // namespace hadron