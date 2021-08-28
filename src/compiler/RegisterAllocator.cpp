#include "hadron/RegisterAllocator.hpp"

#include "hadron/HIR.hpp"
#include "hadron/LifetimeInterval.hpp"
#include "hadron/LinearBlock.hpp"

#include <algorithm>

/*
Pseudocode for the Linear Scan algorithm copied verbatim from [RA4] "Optimized interval splitting in a linear scan
register allocator", by C. Wimmer and H. Mössenböck.

LINEARSCAN
    unhandled = list of intervals sorted by increasing start positions
    active = { }; inactive = { }; handled = { };

    while unhandled =/= { } do
        current = pick and remove first interval from unhandled
        position = start position of current

        // check for intervals in active that are handled or inactive
        for each interval it in active do
            if it ends before position then
                move it from active to handled
            else if it does not cover position then
                move it from active to inactive

        // check for intervals in inactive that are handled or active
        for each interval it in inactive do
            if it ends before position then
                move it from inactive to handled
            else if it covers position then
                move it from inactive to active

        // find a register for current
        TRYALLOCATEFREEREG
        if allocation failed then ALLOCATEBLOCKEDREG

        if current has a register assigned then
            add current to active

TRYALLOCATEFREEREG
    set freeUntilPos of all physical registers to maxInt

    for each interval it in active do
        freeUntilPos[it.reg] = 0

    for each interval it in inactive intersecting with current do
        freeUntilPos[it.reg] = next intersection of it with current

    reg = register with highest freeUntilPos
    if freeUntilPos[reg] = 0 then
        // no register available without spilling
        allocation failed
    else if current ends before freeUntilPos[reg] then
        // register available for the whole interval
        current.reg = reg
    else
        // register available for the first part of the interval
        current.reg = reg
        split current before freeUntilPos[reg]

ALLOCATEBLOCKEDREG
    set nextUsePos of all physical registers to maxInt

    for each interval it in active do
        nextUsePos[it.reg] = next use of it after start of current

    for each interval it in inactive intersecting with current do
        nextUsePos[it.reg] = next use of it after start of current

    reg = register with highest nextUsePos
    if first usage of current is after nextUsePos[reg] then
        // all other intervals are used before current, so it is best to spill current itself
        assign spill slot to current
        split current before its first use position that requires a register
    else
        // spill intervals that currently block reg
        current.reg = reg
        split active interval for reg at position
        split any inactive interval for reg at the end of its lifetime hole

    // make sure that current does not intersect with
    // the fixed interval for reg
    if current intersects with the fixed interval for reg then
        split current before this intersection
*/

namespace {
// Comparison operator for making min heaps in m_unhandled and m_inactive, sorted by start time.
struct IntervalCompare {
    bool operator()(const hadron::LifetimeInterval& lt1, const hadron::LifetimeInterval& lt2) const {
        return (lt1.start() < lt2.start());
    }
};
} // namespace

namespace hadron {

void RegisterAllocator::allocateRegisters(LinearBlock* linearBlock) {
    // We build a min-heap of nonemtpy value lifetimes, ordered by start time. Higher-number values are likely to start
    // later in the block, so we add them to the heap in reverse order.
    m_unhandled.reserve(linearBlock->valueLifetimes.size());
    for (int i = linearBlock->valueLifetimes.size() - 1; i >= 0; --i) {
        if (!linearBlock->valueLifetimes[i][0].isEmpty()) {
            m_unhandled.emplace_back(std::move(linearBlock->valueLifetimes[i][0]));
            linearBlock->valueLifetimes[i].clear();
        }
    }
    std::make_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());

    // unhandled = list of intervals sorted by increasing start positions
    // active = { }; inactive = { }; handled = { };

    // Move any register allocations from block construction into the inactive map.
    m_numberOfRegisters = linearBlock->registerLifetimes.size();
    for (size_t i = 0; i < m_numberOfRegisters; ++i) {
        assert(!linearBlock->registerLifetimes[i][0].isEmpty());
        m_inactive.emplace(std::make_pair(i, std::move(linearBlock->registerLifetimes[i][0])));
        linearBlock->registerLifetimes[i].clear();
    }

    // while unhandled =/= { } do
    while (m_unhandled.size()) {
        // current = pick and remove first interval from unhandled
        std::pop_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        LifetimeInterval current = m_unhandled.back();
        m_unhandled.pop_back();

        // position = start position of current
        size_t position = current.start();
        std::unordered_map<size_t, LifetimeInterval> activeToInactive;

        // check for intervals in active that are handled or inactive
        // for each interval it in active do
        auto iter = m_active.begin();
        while (iter != m_active.end()) {
            // if it ends before position then
            if (iter->second.end() <= position) {
                // move it from active to handled
                handled(iter->second, linearBlock);
                iter = m_active.erase(iter);
            } else {
                // else if it does not cover position then
                if (!iter->second.covers(position)) {
                    // move it from active to inactive
                    activeToInactive.insert(m_active.extract(iter));
                }
                ++iter;
            }
        }

        // check for intervals in inactive that are handled or active
        // for each interval it in inactive do
        iter = m_inactive.begin();
        while (iter != m_inactive.end()) {
            // if it ends before position then
            if (iter->second.end() <= position) {
                // move it from inactive to handled
                handled(iter->second, linearBlock);
                iter = m_active.erase(iter);
            } else {
                // else if it covers position then
                if (iter->second.covers(position)) {
                    // move it from inactive to active
                    m_active.insert(m_inactive.extract(iter));
                }
                ++iter;
            }
        }

        // We save the intervals moving from active to inactive to a temporary map, to avoid a redundant check on those
        // intervals while going through the inactive map. Now move them back to the inactive map.
        m_inactive.merge(std::move(activeToInactive));

        // find a register for current
        // TRYALLOCATEFREEREG
        if (!tryAllocateFreeReg(current)) {
            // if allocation failed then ALLOCATEBLOCKEDREG
            allocateBlockedReg(current, linearBlock);
        }
    }

    // Append any final lifetimes to the linearBlock.
    for (auto iter : m_active) {
        handled(iter.second, linearBlock);
    }
    for (auto iter : m_inactive) {
        handled(iter.second, linearBlock);
    }
    for (auto iter : m_activeSpills) {
        linearBlock->spillLifetimes[iter.first].emplace_back(iter.second);
        linearBlock->valueLifetimes[iter.second.valueNumber].emplace_back(iter.second);
    }
}

bool RegisterAllocator::tryAllocateFreeReg(LifetimeInterval& current) {
    // set freeUntilPos of all physical registers to maxInt
    std::vector<size_t> freeUntilPos(m_numberOfRegisters, std::numeric_limits<size_t>::max());

    // for each interval it in active do
    for (const auto& it : m_active) {
        // freeUntilPos[it.reg] = 0
        freeUntilPos[it.first] = 0;
    }

    // for each interval it in inactive intersecting with current do
    for (const auto& it : m_inactive) {
        size_t nextIntersection = 0;
        if (it.second.findFirstIntersection(current, nextIntersection)) {
            // freeUntilPos[it.reg] = next intersection of it with current
            freeUntilPos[it.first] = nextIntersection;
        }
    }

    // reg = register with highest freeUntilPos
    size_t reg = 0;
    size_t highestFreeUntilPos = 0;
    for (size_t i = 0; reg < freeUntilPos.size(); ++reg) {
        if (freeUntilPos[i] > highestFreeUntilPos) {
            reg = i;
            highestFreeUntilPos = freeUntilPos[i];
        }
    }

    // if freeUntilPos[reg] = 0 then
    if (highestFreeUntilPos == 0) {
        // no register available without spilling
        // allocation failed
        return false;
    } else if (current.end() <= highestFreeUntilPos) {
        // else if current ends before freeUntilPos[reg] then
        //   // register available for the whole interval
        //   current.reg = reg
        current.registerNumber = reg;
    } else {
        // else
        //   // register available for the first part of the interval
        //   current.reg = reg
        current.registerNumber = reg;
        // split current before freeUntilPos[reg]
        m_unhandled.emplace_back(current.splitAt(highestFreeUntilPos));
        std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
    }

    return true;
}

void RegisterAllocator::allocateBlockedReg(LifetimeInterval& current, LinearBlock* linearBlock) {
    // set nextUsePos of all physical registers to maxInt
    std::vector<size_t> nextUsePos(m_numberOfRegisters, std::numeric_limits<size_t>::max());

    // for each interval it in active do
    for (const auto& it : m_active) {
        // nextUsePos[it.reg] = next use of it after start of current
        auto nextUse = it.second.usages.upper_bound(current.start());
        if (nextUse != it.second.usages.end()) {
            nextUsePos[it.first] = *nextUse;
        } else {
            // If there's not a usage but the register is marked as active, use the end of the active interval to
            // approximate the next use. Could happen at the end of a loop block, for instance.
            nextUsePos[it.first] = it.second.end();
        }
    }

    // for each interval it in inactive intersecting with current do
    for (const auto& it : m_inactive) {
        size_t nextIntersection = 0;
        if (it.second.findFirstIntersection(current, nextIntersection)) {
            // nextUsePos[it.reg] = next use of it after start of current
            auto nextUse = it.second.usages.upper_bound(current.start());
            if (nextUse != it.second.usages.end()) {
                nextUsePos[it.first] = *nextUse;
            } else {
                nextUsePos[it.first] = it.second.end();
            }
        }
    }

    // reg = register with highest nextUsePos
    size_t reg = 0;
    size_t highestNextUsePos = 0;
    for (size_t i = 0; reg < nextUsePos.size(); ++reg) {
        if (nextUsePos[i] > highestNextUsePos) {
            reg = i;
            highestNextUsePos = nextUsePos[i];
        }
    }

    // if first usage of current is after nextUsePos[reg] then
    size_t currentFirstUsage = *current.usages.begin();
    if (currentFirstUsage > highestNextUsePos) {
        // all other intervals are used before current, so it is best to spill current itself
        // assign spill slot to current
        // split current before its first use position that requires a register
        m_unhandled.emplace_back(current.splitAt(currentFirstUsage));
        std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        spill(current, linearBlock);
    } else {
        // else
        // spill intervals that currently block reg
        // current.reg = reg
        current.registerNumber = reg;
        auto iter = m_active.find(reg);
        if (iter != m_active.end()) {
            // split active interval for reg at position
            auto activeSpill = iter->second.splitAt(current.start());
            handled(iter->second, linearBlock);
            m_unhandled.emplace_back(activeSpill.splitAt(highestNextUsePos));
            std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
            spill(activeSpill, linearBlock);
            iter->second = current;
        } else {
            iter = m_inactive.find(reg);
            assert(iter != m_inactive.end());
            // split any inactive interval for reg at the end of its lifetime hole
            auto inactiveSpill = iter->second.splitAt(current.start());
            handled(iter->second, linearBlock);
            // TODO: any harm in splitting here instead of at the start of the lifetime hole?
            m_unhandled.emplace_back(inactiveSpill.splitAt(highestNextUsePos));
            std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
            spill(inactiveSpill, linearBlock);
            m_active.emplace(std::make_pair(reg, current));
            m_inactive.erase(iter);
        }

        // TODO: This below pseudocode fixes up a register assignment to current in the event that it collides with a
        // future blocked use of the register, such as in the event that the register is blocked for a function call.
        // Right now Hadron reserves *all* registers for the callee, saving everything in memory for each dispatch. It
        // may make more sense to adopt a calling convention later that reserves some registers for the caller, in which
        // case this implementation will need to be refactored to support blocking some registers only during function
        // calls. For now the blocks in inactive will get moved to unhandled, and may get assigned to another register,
        // but because there is a block for every register it is assumed the allocator will always preserve all
        // registers across calls.

        // make sure that current does not intersect with the fixed interval for reg
        // if current intersects with the fixed interval for reg then
        //   split current before this intersection
    }
}

void RegisterAllocator::spill(LifetimeInterval& interval, LinearBlock* linearBlock) {
    // Update our active spill map in case we can re-use any spill slot no longer needed.
    auto iter = m_activeSpills.begin();
    while (iter != m_activeSpills.end()) {
        if (iter->second.end() <= interval.start()) {
            m_freeSpills.emplace(iter->first);
            linearBlock->spillLifetimes[iter->first].emplace_back(iter->second);
            linearBlock->valueLifetimes[iter->second.valueNumber].emplace_back(iter->second);
            iter = m_activeSpills.erase(iter);
        }
    }
    size_t spillSlot;
    if (m_freeSpills.size()) {
        spillSlot = *m_freeSpills.begin();
        m_freeSpills.erase(m_freeSpills.begin());
    } else {
        spillSlot = linearBlock->numberOfSpillSlots;
        ++linearBlock->numberOfSpillSlots;
        linearBlock->spillLifetimes.emplace_back(std::vector<LifetimeInterval>());
    }
    // Ensure we are reserving spill slot 0 for move cycles.
    assert(spillSlot > 0);

    // Add spill instruction to moves list.
    linearBlock->instructions[interval.start()]->moves.emplace(std::make_pair(interval.registerNumber,
        -static_cast<int>(spillSlot)));

    interval.isSpill = true;
    interval.spillSlot = spillSlot;
    m_activeSpills.emplace(std::make_pair(spillSlot, interval));
}

void RegisterAllocator::handled(LifetimeInterval& interval, LinearBlock* linearBlock) {
    assert(!interval.isSpill);
    linearBlock->registerLifetimes[interval.registerNumber].emplace_back(interval);
    // Check if previous lifetime was a spill, to issue unspill commands if needed.
    if (linearBlock->valueLifetimes[interval.valueNumber].size() &&
            linearBlock->valueLifetimes[interval.valueNumber].back().isSpill) {
        linearBlock->instructions[interval.start()]->moves.emplace(std::make_pair(
            -static_cast<int>(linearBlock->valueLifetimes[interval.valueNumber].back().spillSlot),
            interval.registerNumber));
    }
    linearBlock->valueLifetimes[interval.valueNumber].emplace_back(interval);
    // Update map at every hir for this value number.
    for (size_t i = interval.start(); i < interval.end(); ++i) {
        // TODO: do we need this covers check? Is it harmless to cache reg assignments during lifetime holes?
        if (interval.covers(i)) {
            linearBlock->instructions[i]->valueLocations.emplace(std::make_pair(interval.valueNumber,
                static_cast<int>(interval.registerNumber)));
        }
    }
}

} // namespace hadron