#include "hadron/LifetimeAnalyzer.hpp"

#include "hadron/library/Array.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/ThreadContext.hpp"

#include "spdlog/spdlog.h"

#include <unordered_set>

namespace hadron {

/*
Pseudocode for the lifetime interval building algorithm taken verbatiim from [RA5] in the Bibliography, "Linear Scan
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

void LifetimeAnalyzer::buildLifetimes(ThreadContext* context, library::LinearFrame linearFrame) {
    // Compute blockRanges.
    auto blockRanges = library::TypedArray<library::LiveRange>::typedNewClear(context,
            linearFrame.blockLabels().size());
    int32_t blockStart = 0;
    library::LabelLIR lastLabel = library::LabelLIR();
    for (int32_t i = 0; i < linearFrame.instructions().size(); ++i) {
        auto lir = linearFrame.instructions().typedAt(i);
        if (lir.className() == library::LabelLIR::nameHash()) {
            if (lastLabel) {
                blockRanges.typedPut(lastLabel.labelId().int32(),
                        library::LiveRange::makeLiveRange(context, blockStart, i));
            }
            lastLabel = hadron::library::LabelLIR(lir.slot());
            blockStart = i;
        }
    }
    // Save final block range.
    blockRanges.typedPut(lastLabel.labelId().int32(), library::LiveRange::makeLiveRange(context, blockStart,
            linearFrame.instructions().size()));
    linearFrame.setBlockRanges(blockRanges);

    // Initialize valueLifetimes with arrays each containing one empty LifetimeInterval structure, each with the
    // corresponding labeled valueNumber.
    auto valueLifetimes = library::LinearFrame::Intervals::typedArrayAlloc(context, linearFrame.vRegs().size());
    for (int32_t i = 0; i < linearFrame.vRegs().size(); ++i) {
        valueLifetimes = valueLifetimes.typedAdd(context,
                library::TypedArray<library::LifetimeInterval>::typedArrayAlloc(context));
        valueLifetimes.typedPut(i,
                valueLifetimes.typedAt(i).typedAdd(context,
                library::LifetimeInterval::makeLifetimeInterval(context, i)));
    }

    std::vector<std::unordered_set<int32_t>> liveIns(linearFrame.blockOrder().size());

    // for each block b in reverse order do
    for (int32_t i = linearFrame.blockOrder().size() - 1; i >= 0; --i) {
        int32_t blockNumber = linearFrame.blockOrder().typedAt(i).int32();
        auto blockRange = linearFrame.blockRanges().typedAt(blockNumber);
        auto blockLabel = library::LabelLIR(linearFrame.instructions().typedAt(blockRange.from().int32()).slot());

        // live = union of successor.liveIn for each successor of b
        std::unordered_set<int32_t> live;
        for (int32_t j = 0; j < blockLabel.successors().size(); ++j) {
            auto succNumber = blockLabel.successors().typedAt(j).int32();
            live.insert(liveIns[succNumber].begin(), liveIns[succNumber].end());

            auto succRange = linearFrame.blockRanges().typedAt(succNumber);
            auto succLabel = library::LabelLIR(linearFrame.instructions().typedAt(succRange.from().int32()).slot());

            int inputNumber = 0;
            for (; inputNumber < succLabel.predecessors().size(); ++inputNumber) {
                auto succPred = succLabel.predecessors().typedAt(inputNumber).int32();
                if (succPred == blockNumber) {
                    break;
                }
            }

            // for each phi function phi of successors of b do
            //   live.add(phi.inputOf(b))
            for (int32_t k = 0; k < succLabel.phis().size(); ++k) {
                auto phi = library::PhiLIR(succLabel.phis().typedAt(k).slot());
                live.insert(phi.inputs().typedAt(inputNumber).int32());
            }
        }

        // The next part of the algorithm adds live ranges to the variables used within the block. One operation calls
        // for a modification of a lifetime range (setFrom). Our Lifetime structure doesn't currently support modifying
        // ranges once added, so we save temporary ranges here until final and add them all in then.
        std::vector<std::pair<int32_t, int32_t>> blockVariableRanges(valueLifetimes.size(),
                std::make_pair(std::numeric_limits<int32_t>::max(), 0));

        // for each opd in live do
        for (auto opd : live) {
            // intervals[opd].addRange(b.from, b.to)
            blockVariableRanges[opd] = std::make_pair(blockRange.from().int32(), blockRange.to().int32());
        }

        // for each operation op of b in reverse order do
        for (int32_t j = blockRange.to().int32() - 1; j >= blockRange.from().int32(); --j) {
            assert(0 <= j && j < linearFrame.instructions().size());
            auto lir = linearFrame.instructions().typedAt(j);

            // note: In Hadron there's at most 1 valid output from an LIR so this for loop is instead an if statement.
            // for each output operand opd of op do
            if (lir.vReg()) {
                // intervals[opd].setFrom(op.id)
                blockVariableRanges[lir.vReg().int32()].first = j;
                valueLifetimes.typedAt(lir.vReg().int32()).typedAt(0).usages().add(context, library::Integer(j).slot());

                // live.remove(opd)
                live.erase(lir.vReg().int32());
            }

            // for each input operand opd of op do
            library::VReg opd = lir.reads().typedNext(library::VReg());
            while (opd) {
                // intervals[opd].addRange(b.from, op.id)
                blockVariableRanges[opd.int32()].first = blockRange.from().int32();
                blockVariableRanges[opd.int32()].second = std::max(j + 1, blockVariableRanges[opd.int32()].second);
                valueLifetimes.typedAt(opd.int32()).typedAt(0).usages().add(context, library::Integer(j).slot());
                // live.add(opd)
                live.insert(opd.int32());

                opd = lir.reads().typedNext(opd);
            }
        }

        // for each phi function phi of b do
        for (int32_t j = 0; j < blockLabel.phis().size(); ++j) {
            auto phi = blockLabel.phis().typedAt(j);
            // live.remove(phi.output)
            live.erase(phi.vReg().int32());
        }

        // TODO: loop header step
        // if b is loop header then
        //   loopEnd = last block of the loop starting at b
        //   for each opd in live do
        //     intervals[opd].addRange(b.from, loopEnd.to)

        // b.liveIn = live
        liveIns[blockNumber].swap(live);

        // Cleanup step, add the now final ranges into the lifetimes.
        for (size_t j = 0; j < blockVariableRanges.size(); ++j) {
            if (blockVariableRanges[j].first != std::numeric_limits<int32_t>::max()) {
                // It's possible a value is created in this block but not read. This could be a sign of code that needs
                // more optimization, or a value that's needed in a subsequent block. Record this as a range only around
                // the usage.
                if (blockVariableRanges[j].second == 0) {
                    blockVariableRanges[j].second = blockVariableRanges[j].first + 1;
                }
                assert(blockVariableRanges[j].second > blockVariableRanges[j].first);
                valueLifetimes.typedAt(j).typedAt(0).addLiveRange(context, blockVariableRanges[j].first,
                        blockVariableRanges[j].second);
            }
        }
    }

    linearFrame.setValueLifetimes(valueLifetimes);
}

} // namespace hadron