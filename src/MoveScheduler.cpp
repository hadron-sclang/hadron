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
        iter = processMove(moves, jit, iter);
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

std::unordered_map<int, int>::iterator MoveScheduler::processMove(const std::unordered_map<int, int>& moves, JIT* jit,
        std::unordered_map<int, int>::iterator iter) {
    // Look for the destination in the origins map.
    const auto& moveIter = moves.find(iter->first);
    // Base case is that this destination is not the origin for any other move, so it can be safely scheduled.
    if (moveIter == moves.end()) {
        move(iter->second, iter->first, jit);
        return m_reverseMoves.erase(iter);
    } else {
        // We can resolve a simple cycle (x1 -> x2, x2 -> x1) with the register swap function
    }
}


} // namespace hadron