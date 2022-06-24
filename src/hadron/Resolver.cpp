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

            auto moves = library::TypedIdentDict<Integer, Integer>::makeIdentityDictionary(context);
            // for each interval it live at begin of successor do
            for (auto live : successorLabel->liveIns) {
                int moveFrom, moveTo;

                // if it starts at begin of successor then
                const lir::PhiLIR* livePhi = nullptr;
                for (const auto& phi : successorLabel->phis) {
                    if (phi->value.number == live) {
                        livePhi = phi.get();
                        break;
                    }
                }
                if (livePhi) {
                    // phi = phi function defining it
                    // opd = phi.inputOf(predecessor)
                    size_t opd = livePhi->inputs[succPredIndex].number;
                    // if opd is a constant then TODO
                    //   moveFrom = opd
                    // else
                    //  moveFrom = location of intervals[opd] at end of predecessor
                    bool found = findAt(opd, linearFrame, blockRange.second - 1, moveFrom);
                    assert(found);
                } else {
                    // else
                    // moveFrom = location of it at end of predecessor
                    bool found = findAt(live, linearFrame, blockRange.second - 1, moveFrom);
                    assert(found);
                }
                // moveTo = location of it at begin of successor
                bool found = findAt(live, linearFrame, blockRange.first, moveTo);
                assert(found);
                if (moveFrom != moveTo) {
                    auto emplace = moves.emplace(std::make_pair(moveFrom, moveTo));
                    if (!emplace.second) {
                        // Redundant moves are OK as long as they are to same destination.
                        assert(emplace.first->second == moveTo);
                    }
                }
            }

            if (moves.size()) {
                // If the block has only one successor, or this is the last successor, we can simply prepend the move
                // instructions to the outbound branch.
                if (blockLabel->successors.size() == 1 || successorNumber == blockLabel->successors.back()) {
                    // A block with a successor should never be the last block in the frame.
                    assert(blockNumber + 1 < static_cast<int32_t>(linearFrame->blockLabels.size()));
                    auto lastInstruction = linearFrame->blockLabels[blockNumber + 1];
                    --lastInstruction;
                    // Last instruction should always be a branch.
                    assert((*lastInstruction)->opcode == lir::kBranch);
                    auto branch = reinterpret_cast<lir::BranchLIR*>(lastInstruction->get());
                    branch->moves.merge(std::move(moves));
                } else if (successorLabel->predecessors.size() == 1) {
                    // If the successor has only the predecessor we can prepend the move instructions in the successor
                    // label.
                    successorLabel->moves.merge(std::move(moves));
                } else {
                    // TODO: probably need to insert an additional block with the move instructions before successor,
                    // and have every other incoming edge jump past that.
                    assert(false);
                }
            }
        }
    }
}

bool Resolver::findAt(size_t valueNumber, LinearFrame* linearFrame, size_t line, int& location) {
    for (const auto& lifetime : linearFrame->valueLifetimes[valueNumber]) {
        if (lifetime->start() <= line && line < lifetime->end()) {
            if (!lifetime->isSpill) {
                location = static_cast<int>(lifetime->registerNumber);
            } else {
                location = -1 - static_cast<int>(lifetime->spillSlot);
            }
            return true;
        }
    }
    return false;
}

} // namespace hadron