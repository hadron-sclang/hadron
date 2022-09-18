#include "hadron/Resolver.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/library/HadronLIR.hpp"
#include "hadron/ThreadContext.hpp"

#include <cassert>
#include <unordered_map>

/*
Pseudocode taken from [RA5] "Linear Scan Register Allocation on SSA Form." by C. Wimmer and M. Franz.

RESOLVE
for each control flow edge from predecessor to successor do
    for each interval it live at begin of successor do
        if it starts at begin of successor then
            phi = phi function defining it
            opd = phi.inputOf(predecessor)
            if opd is a constant then
                moveFrom = opd
            else
                moveFrom = location of intervals[opd] at end of predecessor
        else
            moveFrom = location of it at end of predecessor
        moveTo = location of it at begin of successor
        if moveFrom ≠ moveTo then
            mapping.add(moveFrom, moveTo)

    mapping.orderAndInsertMoves()
*/

namespace hadron {

void Resolver::resolve(ThreadContext* context, library::LinearFrame linearFrame) {
    for (int32_t blockIndex = 0; blockIndex < linearFrame.blockOrder().size(); ++blockIndex) {
        auto blockNumber = linearFrame.blockOrder().typedAt(blockIndex).int32();
        auto blockLabel = linearFrame.blockLabels().typedAt(blockNumber);
        auto blockRange = linearFrame.blockRanges().typedAt(blockNumber);

        // for each control flow edge from predecessor to successor do
        for (int32_t successorIndex = 0; successorIndex < blockLabel.successors().size(); ++successorIndex) {
            auto successorNumber = blockLabel.successors().typedAt(successorIndex).int32();
            auto successorLabel = linearFrame.blockLabels().typedAt(successorNumber);
            int32_t succPredIndex = 0;
            for (; succPredIndex < successorLabel.predecessors().size(); ++succPredIndex) {
                if (successorLabel.predecessors().typedAt(succPredIndex).int32() == blockNumber) {
                    break;
                }
            }
            assert(succPredIndex < successorLabel.predecessors().size());

            auto moves = library::TypedIdentDict<library::Integer, library::Integer>::makeTypedIdentDict(context);

            // for each interval it live at begin of successor do
            auto liveIns = linearFrame.liveIns().typedAt(successorNumber);
            auto it = liveIns.typedNext(library::VReg());
            while (it) {
                int32_t moveFrom, moveTo;

                // if it starts at begin of successor then
                auto phi = library::PhiLIR();
                for (int32_t i = 0; i < successorLabel.phis().size(); ++i) {
                    auto succPhi = successorLabel.phis().typedAt(i);
                    if (succPhi.vReg() == it) {
                        // phi = phi function defining it
                        phi = library::PhiLIR::wrapUnsafe(succPhi.slot());
                        break;
                    }
                }
                if (phi) {
                    // opd = phi.inputOf(predecessor)
                    auto opd = phi.inputs().typedAt(succPredIndex).int32();
                    // if opd is a constant then TODO
                    //   moveFrom = opd
                    // else
                    //  moveFrom = location of intervals[opd] at end of predecessor
                    bool found = findAt(opd, linearFrame, blockRange.to().int32() - 1, moveFrom);
                    assert(found);
                } else {
                    // else
                    // moveFrom = location of it at end of predecessor
                    bool found = findAt(it.int32(), linearFrame, blockRange.to().int32() - 1, moveFrom);
                    assert(found);
                }
                // moveTo = location of it at begin of successor
                bool found = findAt(it.int32(), linearFrame, blockRange.from().int32(), moveTo);
                assert(found);

                // if moveFrom ≠ moveTo then
                if (moveFrom != moveTo) {
                    // mapping.add(moveFrom, moveTo)
                    moves.typedPut(context, moveFrom, moveTo);
                }

                it = liveIns.typedNext(it);
            }

            // If we have moves scheduled we need to add them to some some instruction in the LinearFrame.
            if (moves.size()) {
                // If the block has only one successor, or this is the last successor, we can simply prepend the move
                // instructions to the outbound branch.
                if (blockLabel.successors().size() == 1
                    || successorNumber == blockLabel.successors().typedLast().int32()) {
                    // Last instruction should always be an unconditional branch.
                    auto lastInstruction = blockRange.to().int32() - 1;
                    auto branch = library::BranchLIR(linearFrame.instructions().typedAt(lastInstruction).slot());
                    branch.moves().typedPutAll(context, moves);
                } else if (successorLabel.predecessors().size() == 1) {
                    // If the successor has only the predecessor we can prepend the move instructions in the successor
                    // label.
                    successorLabel.moves().typedPutAll(context, moves);
                } else {
                    // TODO: probably need to insert an additional block with the move instructions before successor,
                    // and have every other incoming edge jump past that.
                    assert(false);
                }
            }
        }
    }
}

bool Resolver::findAt(int32_t valueNumber, library::LinearFrame linearFrame, int32_t line, int32_t& location) {
    for (int32_t i = 0; i < linearFrame.valueLifetimes().typedAt(valueNumber).size(); ++i) {
        auto lifetime = linearFrame.valueLifetimes().typedAt(valueNumber).typedAt(i);
        if (lifetime.start().int32() <= line && line < lifetime.end().int32()) {
            if (!lifetime.isSpill()) {
                location = lifetime.registerNumber().int32();
            } else {
                location = -1 - lifetime.spillSlot().int32();
            }
            return true;
        }
    }
    return false;
}

} // namespace hadron