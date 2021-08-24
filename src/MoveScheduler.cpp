#include "hadron/MoveScheduler.hpp"

#include "hadron/HIR.hpp"
#include "hadron/JIT.hpp"
#include "hadron/Slot.hpp"

namespace hadron {

bool MoveScheduler::scheduleMoves(const std::unordered_map<int, int>& moves, JIT* jit) {
    std::unordered_map<int, int> reverseMoves;

    // Build the reverse set of destination, origin.
    for (const auto& movePair : moves) {
        auto emplace = reverseMoves.emplace(std::make_pair(movePair.second, movePair.first));
        // Ambiguous move caused by a destination having multiple origins.
        if (!emplace.second) {
            return false;
        }
    }

    auto iter = reverseMoves.begin();
    while (iter != reverseMoves.end()) {
        // Look for the destination of this move in the origins map.
        auto originIter = moves.find(iter->first);
        // Base case is that this destination is not the origin for another move, so it can be safely scheduled.
        if (originIter == moves.end()) {
            move(iter->first, iter->second, jit);
            iter = reverseMoves.erase(iter);
        } else {
            // Destination move may have already been scheduled, look for it in the reverse map.
            auto destIter = reverseMoves.find(originIter->second);
            if (destIter == reverseMoves.end()) {
                move(iter->first, iter->second, jit);
                iter = reverseMoves.erase(iter);
            }  else if (destIter->first == iter->second) {
                // We can resolve a simple cycle (x1 -> x2, x2 -> x1) with the register swap function from [BK1]
                // Hacker's Delight 2.d Ed by Henry S. Warren, Jr.
                jit->xorr(iter->first, iter->first, iter->second);
                jit->xorr(iter->second, iter->second, iter->first);
                jit->xorr(iter->first, iter->first, iter->second);
                reverseMoves.erase(destIter);
                iter = reverseMoves.erase(iter);
            } else {
                // This is either a chain of copies or a cycle. Extract all the copies into a separate map.
                bool isCycle = false;
                std::unordered_map<int, int> chain;
                chain.insert(reverseMoves.extract(iter));
                auto chainIter = chain.insert(reverseMoves.extract(destIter)).position;
                // Find next link in the chain, if applicable.
                originIter = moves.find(chainIter->first);
                while (originIter != moves.end() && !isCycle) {
                    if (chain.find(originIter->second) != chain.end()) {
                        isCycle = true;
                    } else {
                        chainIter = chain.insert(reverseMoves.extract(originIter->second)).position;
                        originIter = moves.find(originIter->second);
                    }
                }
                if (!isCycle) {
                    // Schedule from the end of the chain back to the beginning.
                    while (chainIter != chain.end()) {
                        move(chainIter->first, chainIter->second, jit);
                        chainIter = chain.find(chainIter->second);
                    }
                } else {
                    // Iterate through cycle until we have a register to save to the temporary slot.
                    auto cycleIter = chainIter;
                    if (cycleIter->second < 0) {
                        do {
                            cycleIter = chain.find(cycleIter->second);
                        } while (cycleIter->second < 0 && cycleIter != chainIter);
                        // Should always be at least register in a copy cycle.
                        assert(cycleIter->second >= 0);
                    }

                    // First emit a move to slot 0, the temporary slot, to save one end of the chain.
                    int registerSaved = cycleIter->first;
                    jit->stxi_w(Slot::slotValueOffset(0), JIT::kStackPointerReg, registerSaved);
                    // Then emit the rest of the copy chain back until the saved register is the source.
                    do {
                        move(cycleIter->first, cycleIter->second, jit);
                        cycleIter = chain.find(cycleIter->second);
                    } while (registerSaved != cycleIter->second);
                    // Restore the saved register to its destination.
                    jit->ldxi_w(cycleIter->first, JIT::kStackPointerReg, offsetof(Slot, value));
                }

                // Continue at the start of the map.
                iter = reverseMoves.begin();
            }
        }
    }

    return true;
}

void MoveScheduler::move(int destination, int origin, JIT* jit) {
    if (origin >= 0) {
        if (destination >= 0) {
            // Moving from a register to a register.
            jit->movr(destination, origin);
        } else {
            // Moving from a register to a spill slot location.
            jit->stxi_w(Slot::slotValueOffset(destination), JIT::kStackPointerReg, origin);
        }
    } else {
        if (destination >= 0) {
            // Moving from spill slot location to a register.
            jit->ldxi_w(destination, JIT::kStackPointerReg, Slot::slotValueOffset(origin));
        } else {
            // Moving from spill to spill. Hard to do without a temporary register.
            assert(false);
        }
    }
}

} // namespace hadron