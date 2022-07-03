#include "hadron/MoveScheduler.hpp"

#include "hadron/JIT.hpp"
#include "hadron/Slot.hpp"

#include <unordered_map>

namespace hadron {

bool MoveScheduler::scheduleMoves(library::TypedIdentDict<library::Integer, library::Integer> moves, JIT* jit) {
    std::unordered_map<int32_t, int32_t> reverseMoves;

    // Build the reverse set of destination, origin.
    for (int32_t i = 0; i < moves.array().size(); i += 2) {
        auto origin = moves.array().at(i);
        if (origin) {
            auto destination = moves.array().at(i + 1);
            auto emplace = reverseMoves.emplace(std::make_pair(destination.getInt32(), origin.getInt32()));
            // Ambiguous move caused by a destination having multiple origins.
            assert(emplace.second);
        }
    }

    auto iter = reverseMoves.begin();
    while (iter != reverseMoves.end()) {
        // Look for the destination of this move in the origins map.
        auto origin = moves.typedGet(library::Integer(iter->first));

        // Base case is that this destination is not the origin for another move, so it can be safely scheduled.
        if (!origin) {
            move(iter->first, iter->second, jit);
            iter = reverseMoves.erase(iter);
            continue;
        }

        // Destination move may have already been scheduled, look for it in the reverse map.
        auto destIter = reverseMoves.find(origin.int32());
        if (destIter == reverseMoves.end()) {
            move(iter->first, iter->second, jit);
            iter = reverseMoves.erase(iter);
            continue;
        }

        // We can resolve a simple cycle (x1 -> x2, x2 -> x1) with the register swap function from [BK1] Hacker's
        // Delight 2.d Ed by Henry S. Warren, Jr.
        if (destIter->first == iter->second) {
            jit->xorr(iter->first, iter->first, iter->second);
            jit->xorr(iter->second, iter->second, iter->first);
            jit->xorr(iter->first, iter->first, iter->second);
            reverseMoves.erase(destIter);
            iter = reverseMoves.erase(iter);
            continue;
        }

        // This is either a chain of copies or a cycle. Extract all the copies into a separate map.
        bool isCycle = false;
        std::unordered_map<int32_t, int32_t> chain;
        chain.insert(reverseMoves.extract(iter));
        auto chainIter = chain.insert(reverseMoves.extract(destIter)).position;

        // Find next link in the chain, if applicable.
        origin = moves.typedGet(library::Integer(chainIter->first));
        while (origin && !isCycle) {
            if (chain.find(origin.int32()) != chain.end()) {
                isCycle = true;
            } else {
                chainIter = chain.insert(reverseMoves.extract(origin.int32())).position;
                origin = moves.typedGet(origin);
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
            // TODO: this will likely require repairs for 32-bit.
            jit->str_l(JIT::kStackPointerReg, registerSaved);
            // Then emit the rest of the copy chain back until the saved register is the source.
            do {
                move(cycleIter->first, cycleIter->second, jit);
                cycleIter = chain.find(cycleIter->second);
            } while (registerSaved != cycleIter->second);
            // Restore the saved register to its destination.
            jit->ldr_l(cycleIter->first, JIT::kStackPointerReg);
        }

        // Continue at the start of the map.
        iter = reverseMoves.begin();
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
            jit->stxi_l(destination * kSlotSize, JIT::kStackPointerReg, origin);
        }
    } else {
        if (destination >= 0) {
            // Moving from spill slot location to a register.
            jit->ldxi_l(destination, JIT::kStackPointerReg, origin * kSlotSize);
        } else {
            // Moving from spill to spill. Hard to do without a temporary register.
            assert(false);
        }
    }
}

} // namespace hadron