#include "hadron/RegisterAllocator.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/LifetimeInterval.hpp"

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
            m_unhandled.emplace_back(linearBlock->valueLifetimes[i][0]);
        }
    }
    std::make_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());

    // unhandled = list of intervals sorted by increasing start positions
    // active = { }; inactive = { }; handled = { };

    // Seed any register allocations from block construction into the inactive map.
    m_numberOfRegisters = linearBlock->registerLifetimes.size();
    for (size_t i = 0; i < m_numberOfRegisters; ++i) {
        assert(!linearBlock->registerLifetimes[i][0].isEmpty());
        m_inactive.emplace(std::make_pair(i, linearBlock->registerLifetimes[i][0]));
    }

    // while unhandled =/= { } do
    while (m_unhandled.size()) {
        // current = pick and remove first interval from unhandled
        std::pop_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        LifetimeInterval current = m_unhandled.back();
        m_unhandled.pop_back();

        // position = start position of current
        size_t position = current.start();

        // check for intervals in active that are handled or inactive
        // for each interval it in active do
        auto iter = m_active.begin();
        while (iter != m_active.end()) {
            // if it ends before position then
            if (iter->second.end() <= position) {
                // move it from active to handled
                // TODO: "handled" for us could be the completed registerLifetimes structure within linearBlock. So
                // some sort of sorted insertion?
                iter = m_active.erase(iter);
            } else {
                // else if it does not cover position then
                if (!iter->second.covers(position)) {
                    // TODO: remove redundant second check, maybe extract into temp vector or something then
                    // insert after?
                    // move it from active to inactive
                    m_inactive.insert(m_active.extract(iter));
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
                // TODO: handled
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

        // find a register for current
        // TRYALLOCATEFREEREG
        if (!tryAllocateFreeReg(current)) {
            // if allocation failed then ALLOCATEBLOCKEDREG
            allocateBlockedReg(current);
        }
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
        m_unhandled.push_back(current.splitAt(highestFreeUntilPos));
    }

    return true;
}

void RegisterAllocator::allocateBlockedReg(LifetimeInterval& current) {
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
            // approximate the next use. This should be a rare error condition?
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
    if (current.start() > highestNextUsePos) {
        // all other intervals are used before current, so it is best to spill current itself
        // assign spill slot to current
        // TODO: spilling
        // split current before its first use position that requires a register
        m_unhandled.push_back(current.splitAt(highestNextUsePos));
    } else {
        // else
        // spill intervals that currently block reg
        // current.reg = reg
        // split active interval for reg at position
        // split any inactive interval for reg at the end of its lifetime hole
    }

    // make sure that current does not intersect with the fixed interval for reg
    // if current intersects with the fixed interval for reg then
        // split current before this intersection

}

} // namespace hadron