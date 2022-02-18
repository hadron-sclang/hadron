#include "hadron/LifetimeAnalyzer.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LabelLIR.hpp"
#include "hadron/lir/LIR.hpp"

#include "spdlog/spdlog.h"

#include <unordered_set>

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

void LifetimeAnalyzer::buildLifetimes(LinearFrame* linearFrame) {
    linearFrame->lineNumbers.reserve(linearFrame->instructions.size());
    linearFrame->blockRanges.resize(linearFrame->blockLabels.size());
    size_t blockStart = 0;
    const lir::LabelLIR* lastLabel = nullptr;
    for (auto& lir : linearFrame->instructions) {
        if (lir->opcode == lir::kLabel) {
            if (lastLabel) {
                linearFrame->blockRanges[lastLabel->id] = std::make_pair(blockStart, linearFrame->lineNumbers.size());
            }
            lastLabel = reinterpret_cast<const lir::LabelLIR*>(lir.get());
            blockStart = linearFrame->lineNumbers.size();
        }
        linearFrame->lineNumbers.emplace_back(lir.get());
    }
    assert(lastLabel);
    // Save final block range.
    linearFrame->blockRanges[lastLabel->id] = std::make_pair(blockStart, linearFrame->lineNumbers.size());

    assert(linearFrame->lineNumbers.size() == linearFrame->instructions.size());

    std::vector<std::unordered_set<size_t>> liveIns(linearFrame->blockOrder.size());
    linearFrame->valueLifetimes.resize(linearFrame->vRegs.size());
    for (size_t i = 0; i < linearFrame->vRegs.size(); ++i) {
        linearFrame->valueLifetimes[i].emplace_back(std::make_unique<LifetimeInterval>());
        linearFrame->valueLifetimes[i][0]->valueNumber = i;
    }

    // for each block b in reverse order do
    for (int i = linearFrame->blockOrder.size() - 1; i >= 0; --i) {
        int blockNumber = linearFrame->blockOrder[i];
        auto blockRange = linearFrame->blockRanges[blockNumber];
        assert(linearFrame->lineNumbers[blockRange.first]->opcode == lir::kLabel);
        auto blockLabel = reinterpret_cast<lir::LabelLIR*>(linearFrame->lineNumbers[blockRange.first]);

        // live = union of successor.liveIn for each successor of b
        std::unordered_set<size_t> live;
        for (auto succNumber : blockLabel->successors) {
            auto succRange = linearFrame->blockRanges[succNumber];
            assert(linearFrame->lineNumbers[succRange.first]->opcode == lir::kLabel);
            const auto succLabel = reinterpret_cast<const lir::LabelLIR*>(
                    linearFrame->lineNumbers[succRange.first]);

            live.insert(liveIns[succNumber].begin(), liveIns[succNumber].end());

            int inputNumber = 0;
            for (auto succPred : succLabel->predecessors) {
                if (succPred == blockNumber) {
                    break;
                }
                ++inputNumber;
            }
            // for each phi function phi of successors of b do
            //   live.add(phi.inputOf(b))
            for (const auto& lir : succLabel->phis) {
                assert(lir->opcode == lir::kPhi);
                const auto phi = reinterpret_cast<lir::PhiLIR*>(lir.get());
                live.insert(phi->inputs[inputNumber]);
            }
        }

        // The next part of the algorithm adds live ranges to the variables used within the block. One operation calls
        // for a modification of a lifetime range (setFrom). Our Lifetime structure doesn't currently support modifying
        // ranges once added, so we save temporary ranges here until final and add them all in then.
        std::vector<std::pair<size_t, size_t>> blockVariableRanges(linearFrame->valueLifetimes.size(),
                std::make_pair(std::numeric_limits<size_t>::max(), 0));

        // for each opd in live do
        for (auto opd : live) {
            // intervals[opd].addRange(b.from, b.to)
            blockVariableRanges[opd] = blockRange;
        }

        // for each operation op of b in reverse order do
        for (int j = static_cast<int>(blockRange.second) - 1; j >= static_cast<int>(blockRange.first); --j) {
            assert(0 <= j && j < static_cast<int>(linearFrame->instructions.size()));
            const lir::LIR* lir = linearFrame->lineNumbers[j];
            // In Hadron there's at most 1 valid output from an LIR so this for loop is instead an if statement.
            // for each output operand opd of op do
            if (lir->value != lir::kInvalidVReg) {
                // intervals[opd].setFrom(op.id)
                blockVariableRanges[lir->value].first = static_cast<size_t>(j);
                linearFrame->valueLifetimes[lir->value][0]->usages.emplace(static_cast<size_t>(j));

                // live.remove(opd)
                live.erase(lir->value);
            }

            // for each input operand opd of op do
            for (auto opd : lir->reads) {
                // intervals[opd].addRange(b.from, op.id)
                blockVariableRanges[opd].first = blockRange.first;
                blockVariableRanges[opd].second = std::max(static_cast<size_t>(j + 1), blockVariableRanges[opd].second);
                linearFrame->valueLifetimes[opd][0]->usages.emplace(j);
                // live.add(opd)
                live.insert(opd);
            }
        }

        // for each phi function phi of b do
        for (const auto& phi : blockLabel->phis) {
            // live.remove(phi.output)
            live.erase(phi->value);
        }

        // TODO: loop header step
        // if b is loop header then
        //   loopEnd = last block of the loop starting at b
        //   for each opd in live do
        //     intervals[opd].addRange(b.from, loopEnd.to)

        // b.liveIn = live
        liveIns[blockNumber].swap(live);

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
                linearFrame->valueLifetimes[j][0]->addLiveRange(blockVariableRanges[j].first,
                        blockVariableRanges[j].second);
            }
        }
    }
}

} // namespace hadron