#include "hadron/MoveScheduler.hpp"

#include "hadron/HIR.hpp"
#include "hadron/JIT.hpp"
#include "hadron/Slot.hpp"

#include <unordered_set>

namespace hadron {

bool MoveScheduler::scheduleMoves(const std::unordered_map<int, int>& moves, JIT* jit) {
    // Build the reverse set of destination, origin.
    for (const auto& movePair : moves) {
        auto emplace = m_reverseMoves.emplace(std::make_pair(movePair.second, movePair.first));
        // Ambiguous move caused by a destination having multiple origins.
        if (!emplace.second) {
            return false;
        }
    }

    auto iter = m_reverseMoves.begin();
    while (iter != m_reverseMoves.end()) {
        // Look for the destination in the origins map.
        std::unordered_map<int, int>::const_iterator moveIter = moves.find(iter->first);
        // Base case is that this destination is not the origin for any other move, so it can be safely scheduled.
        if (moveIter == moves.end()) {
            move(iter->second, iter->first, jit);
            iter = m_reverseMoves.erase(iter);
        } else {
            // We can resolve a simple cycle (x1 -> x2, x2 -> x1) with the register swap function from [BK1] Hacker's
            // Delight 2.d Ed by Henry S. Warren, Jr.
            if (moveIter->second == iter->second) {
                jit->xorr(iter->first, iter->first, iter->second);
                jit->xorr(iter->second, iter->second, iter->first);
                jit->xorr(iter->first, iter->first, iter->second);
                m_reverseMoves.erase(iter->second);
                iter = m_reverseMoves.erase(iter);
            } else {
                auto backIter = m_reverseMoves.find(iter->second);
                assert(backIter != m_reverseMoves.end());
                // This is either a chain of copies or a cycle. Extract all the copies into a separate map.
                int chainEnd = moveIter->second;
                bool isCycle = false;
                std::unordered_map<int, int> chain;
                chain.emplace(m_reverseMoves.extract(iter));
                chain.emplace(m_reverseMoves.extract(moveIter->second));
                // Find next link in the chain, if applicable.
                moveIter = moves.find(moveIter->second);
                while (moveIter != moves.end() && !isCycle) {
                    if (chain.find(moveIter->second) != chain.end()) {
                        isCycle = true;
                    } else {
                        chainEnd = moveIter->second;
                        chain.emplace(m_reverseMoves.extract(moveIter->second));
                        moveIter = moves.find(moveIter->second);
                    }
                }
                if (!isCycle) {
                } else {
                }

                // Continue at the start of the map.
                iter = m_reverseMoves.begin();
            }
        }
    }

    return true;
}

void MoveScheduler::move(int origin, int destination, JIT* jit) {
    if (origin >= 0) {
        if (destination >= 0) {
            // Moving from a register to a register.
            jit->movr(origin, destination);
        } else {
            // Moving from a register to a spill slot location.
            int spillDestination = -1 - destination;
            jit->stxi_w((spillDestination * -1 * sizeof(Slot)) + offsetof(Slot, value), JIT::kStackPointerReg, origin);
        }
    } else {
        int spillOrigin = -1 - origin;
        if (destination >= 0) {
            // Moving from spill slot location to a register.
            jit->ldxi_w(destination, JIT::kStackPointerReg, (spillOrigin * -1 * sizeof(Slot)) + offsetof(Slot, value));
        } else {
            // Moving from spill to spill. Hard to do without a temporary register.
            assert(false);
        }
    }
}

} // namespace hadron