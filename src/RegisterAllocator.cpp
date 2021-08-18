#include "hadron/RegisterAllocator.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/Lifetime.hpp"

#include <algorithm>

namespace {
// Comparison operator for making min heaps in m_unhandled and m_inactive, sorted by start time.
struct RegIntervalCompare {
    bool operator()(const hadron::RegInterval& r1, const hadron::RegInterval& r2) const {
        return (r1.from < r2.from);
    }
};
} // namespace

namespace hadron {

/*
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

void RegisterAllocator::allocateRegisters(LinearBlock* linearBlock) {
    // Build unhandled as the union of all value lifetimes.
    for (size_t i = 0; i < linearBlock->valueLifetimes.size(); ++i) {
        for (const auto& interval : linearBlock->valueLifetimes[i].intervals) {
            m_unhandled.emplace_back(RegInterval(interval.from, interval.to, i, false));
        }
    }
    std::make_heap(m_unhandled.begin(), m_unhandled.end(), RegIntervalCompare());

    // while unhandled =/= { } do
    while (m_unhandled.size()) {
        // current = pick and remove first interval from unhandled
        std::pop_heap(m_unhandled.begin(), m_unhandled.end(), RegIntervalCompare());
        // RegInterval current = m_unhandled.back();
        m_unhandled.pop_back();

        // position = start position of current (we just use current.from)

        // for each interval it in active do
        //     if it ends before position then
        //         move it from active to handled
        //     else if it does not cover position then
        //         move it from active to inactive

        // this is about the real meaning of *inactive* - we're instead keeping a mapping of register->active value
        // subsequent ranges of value use are just in *unhandled*. We'll clearly want to mark value intervals with
        // reg assignments and spilling, too, just so we can understand where everything is.

        // algo needs a tweak - what goes on the heap is Lifetimes, and they are sorted by first interval (we don't
        // even bother putting empty lifetimes on heap)
        // rename interval to something else (Lifetime?), rename lifetime to interval, to match 

    }

}

} // namespace hadron