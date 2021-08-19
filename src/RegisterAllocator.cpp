#include "hadron/RegisterAllocator.hpp"

#include "hadron/BlockSerializer.hpp"
#include "hadron/LifetimeInterval.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace {
// Comparison operator for making min heaps in m_unhandled and m_inactive, sorted by start time.
struct IntervalCompare {
    bool operator()(const hadron::LifetimeInterval& lt1, const hadron::LifetimeInterval& lt2) const {
        return (lt1.start() < lt2.start());
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
        freeUntilPos[it.reg] = next intersection of it with current   *** TODO: find first intersection operation on Lifetimes

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
    // unhandled = list of intervals sorted by increasing start positions

    // We build a min-heap of nonemtpy value lifetimes, ordered by start time. Higher-number values are likely to start
    // later in the block, so we add them to the heap in reverse order.
    std::vector<LifetimeInterval> unhandled;
    unhandled.reserve(linearBlock->valueLifetimes.size());
    for (int i = linearBlock->valueLifetimes.size() - 1; i >= 0; --i) {
        if (!linearBlock->valueLifetimes[i][0].isEmpty()) {
            unhandled.emplace_back(linearBlock->valueLifetimes[i][0]);
        }
    }
    std::make_heap(unhandled.begin(), unhandled.end(), IntervalCompare());

    // active = { }; inactive = { }; handled = { };

    std::unordered_map<size_t, LifetimeInterval> active;
    std::unordered_map<size_t, LifetimeInterval> inactive;

    // Seed any register allocations from block construction into the inactive.

    // while unhandled =/= { } do
    while (unhandled.size()) {
        // current = pick and remove first interval from unhandled
        std::pop_heap(unhandled.begin(), unhandled.end(), IntervalCompare());
        LifetimeInterval current = unhandled.back();
        unhandled.pop_back();

        // position = start position of current
        size_t position = current.start();

        // for each interval it in active do
        //     if it ends before position then
        //         move it from active to handled
        //     else if it does not cover position then
        //         move it from active to inactive

        // this is about the real meaning of *inactive* - we're instead keeping a mapping of register->active value
        // subsequent ranges of value use are just in *unhandled*. We'll clearly want to mark value intervals with
        // reg assignments and spilling, too, just so we can understand where everything is.
    }
}

} // namespace hadron