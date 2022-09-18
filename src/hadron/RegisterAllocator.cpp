#include "hadron/RegisterAllocator.hpp"

#include "hadron/library/HadronLIR.hpp"
#include "hadron/ThreadContext.hpp"

#include <algorithm>

#include "spdlog/spdlog.h"

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
    bool operator()(const hadron::library::LifetimeInterval& lt1, const hadron::library::LifetimeInterval& lt2) const {
        return (lt1.start().int32() > lt2.start().int32());
    }
};
} // namespace

namespace hadron {

RegisterAllocator::RegisterAllocator(int32_t numberOfRegisters): m_numberOfRegisters(numberOfRegisters) {
    m_active.resize(m_numberOfRegisters);
    m_inactive.resize(m_numberOfRegisters);
}

void RegisterAllocator::allocateRegisters(ThreadContext* context, library::LinearFrame linearFrame) {
    // We build a min-heap of nonemtpy value lifetimes, ordered by start time. Higher-number values are likely to start
    // later in the block, so we add them to the heap in reverse order.
    m_unhandled.reserve(linearFrame.valueLifetimes().size());
    for (int32_t i = linearFrame.valueLifetimes().size() - 1; i >= 0; --i) {
        if (!linearFrame.valueLifetimes().typedAt(i).typedAt(0).isEmpty()) {
            m_unhandled.push_back(linearFrame.valueLifetimes().typedAt(i).typedAt(0));
            linearFrame.valueLifetimes().typedAt(i).resize(context, 0);
        }
    }
    std::make_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());

    // unhandled = list of intervals sorted by increasing start positions
    // active = { }; inactive = { }; handled = { };

    // Populate m_inactive with any register reservations, and add at least one usage for every register at the end
    // of the program, useful for minimizing corner cases in calculation of register next usage during allocation.
    auto numberOfInstructions = linearFrame.instructions().size();
    for (int32_t i = 0; i < m_numberOfRegisters; ++i) {
        auto regLifetime = library::LifetimeInterval::makeLifetimeInterval(context);
        regLifetime.setRegisterNumber(i);
        regLifetime.usages().add(context, Slot::makeInt32(numberOfInstructions));
        regLifetime.addLiveRange(context, numberOfInstructions, numberOfInstructions + 1);
        m_inactive[i].push_back(regLifetime);
    }
    // Iterate through all instructions and add additional reservations as needed.
    for (int32_t i = 0; i < numberOfInstructions; ++i) {
        if (linearFrame.instructions().typedAt(i).shouldPreserveRegisters()) {
            for (int32_t j = 0; j < m_numberOfRegisters; ++j) {
                auto regLifetime = m_inactive[j].back();
                regLifetime.addLiveRange(context, i, i + 1);
                regLifetime.usages().add(context, Slot::makeInt32(i));
            }
        }
    }

    m_activeSpills.resize(linearFrame.numberOfSpillSlots());

    // while unhandled =/= { } do
    while (m_unhandled.size()) {
        // current = pick and remove first interval from unhandled
        std::pop_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        m_current = m_unhandled.back();
        m_unhandled.pop_back();
        assert(!m_current.isEmpty());

        // position = start position of current
        auto position = m_current.start().int32();

        // check for intervals in active that are handled or inactive
        // for each interval it in active do
        for (size_t reg = 0; reg < m_active.size(); ++reg) {
            if (!m_active[reg]) {
                continue;
            }
            // if it ends before position then
            if (m_active[reg].end().int32() <= position) {
                // move it from active to handled
                handled(context, m_active[reg], linearFrame);
                m_active[reg] = library::LifetimeInterval();
            } else {
                // else if it does not cover position then
                if (!m_active[reg].covers(position)) {
                    // move it from active to inactive
                    m_inactive[reg].emplace_back(std::move(m_active[reg]));
                    m_active[reg] = library::LifetimeInterval();
                }
            }
        }

        // check for intervals in inactive that are handled or active
        // for each interval it in inactive do
        for (size_t reg = 0; reg < m_inactive.size(); ++reg) {
            auto iter = m_inactive[reg].begin();
            while (iter != m_inactive[reg].end()) {
                // if it ends before position then
                if (iter->end().int32() <= position) {
                    // move it from inactive to handled
                    handled(context, *iter, linearFrame);
                    iter = m_inactive[reg].erase(iter);
                } else {
                    // else if it covers position then
                    if (iter->covers(position)) {
                        assert(m_active[reg]);
                        m_active[reg] = *iter;
                        iter = m_inactive[reg].erase(iter);
                    } else {
                        ++iter;
                    }
                }
            }
        }

        // find a register for current
        // TRYALLOCATEFREEREG
        if (!tryAllocateFreeReg(context)) {
            // if allocation failed then ALLOCATEBLOCKEDREG
            allocateBlockedReg(context, linearFrame);
        }
    }

    // Append any final lifetimes to the linearFrame.
    for (size_t reg = 0; reg < m_active.size(); ++reg) {
        if (m_active[reg]) {
            if (m_active[reg].valueNumber()) {
                handled(context, m_active[reg], linearFrame);
            }
            m_active[reg] = library::LifetimeInterval();
        }

        auto iter = m_inactive[reg].begin();
        while (iter != m_inactive[reg].end()) {
            if (iter->valueNumber()) {
                handled(context, *iter, linearFrame);
                *iter = library::LifetimeInterval();
                iter = m_inactive[reg].erase(iter);
            } else {
                ++iter;
            }
        }
        m_inactive[reg].clear();
    }

    for (size_t spill = 1; spill < m_activeSpills.size(); ++spill) {
        if (m_activeSpills[spill]) {
            auto spillValue = m_activeSpills[spill].valueNumber().int32();
            linearFrame.valueLifetimes().typedPut(
                spillValue, linearFrame.valueLifetimes().typedAt(spillValue).typedAdd(context, m_activeSpills[spill]));
            m_activeSpills[spill] = library::LifetimeInterval();
        }
    }

    linearFrame.setNumberOfSpillSlots(static_cast<int32_t>(m_activeSpills.size()));
}

bool RegisterAllocator::tryAllocateFreeReg(ThreadContext* context) {
    // set freeUntilPos of all physical registers to maxInt
    std::vector<int32_t> freeUntilPos(m_numberOfRegisters, std::numeric_limits<int32_t>::max());

    // for each interval it in active do
    for (int32_t i = 0; i < m_numberOfRegisters; ++i) {
        if (m_active[i]) {
            // freeUntilPos[it.reg] = 0
            freeUntilPos[i] = 0;
        } else {
            // for each interval it in inactive intersecting with current do
            for (auto& it : m_inactive[i]) {
                int32_t nextIntersection = 0;
                if (it.findFirstIntersection(m_current, nextIntersection)) {
                    // freeUntilPos[it.reg] = next intersection of it with current
                    freeUntilPos[i] = std::min(freeUntilPos[i], nextIntersection);
                }
            }
        }
    }

    // reg = register with highest freeUntilPos
    int32_t reg = 0;
    int32_t highestFreeUntilPos = freeUntilPos[0];
    for (size_t i = 1; i < freeUntilPos.size(); ++i) {
        if (freeUntilPos[i] > highestFreeUntilPos) {
            reg = static_cast<int32_t>(i);
            highestFreeUntilPos = freeUntilPos[i];
        }
    }

    // if freeUntilPos[reg] = 0 then
    if (highestFreeUntilPos == 0) {
        // no register available without spilling
        // allocation failed
        return false;
    } else if (m_current.end().int32() <= highestFreeUntilPos) {
        // else if current ends before freeUntilPos[reg] then
        //   // register available for the whole interval
        //   current.reg = reg
        m_current.setRegisterNumber(reg);
    } else {
        // else
        //   // register available for the first part of the interval
        //   current.reg = reg
        m_current.setRegisterNumber(reg);
        // split current before freeUntilPos[reg]
        m_unhandled.emplace_back(m_current.splitAt(context, highestFreeUntilPos));
        std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
    }

    assert(!m_active[reg]);
    m_active[reg] = m_current;
    m_current = library::LifetimeInterval();
    return true;
}

void RegisterAllocator::allocateBlockedReg(ThreadContext* context, library::LinearFrame linearFrame) {
    // set nextUsePos of all physical registers to maxInt
    std::vector<int32_t> nextUsePos(m_numberOfRegisters, std::numeric_limits<int32_t>::max());

    // for each interval it in active do
    for (int32_t i = 0; i < m_numberOfRegisters; ++i) {
        if (m_active[i]) {
            // nextUsePos[it.reg] = next use of it after start of current
            auto nextUse = m_active[i].usages().lowerBound(m_current.start());
            if (nextUse) {
                nextUsePos[i] = nextUse.int32();
            } else {
                // If there's not a usage but the register is marked as active, use the end of the active interval to
                // approximate the next use. Could happen at the end of a loop block, for instance.
                nextUsePos[i] = m_active[i].end().int32();
            }
        }
        // for each interval it in inactive intersecting with current do
        for (auto& it : m_inactive[i]) {
            int32_t nextIntersection = 0;
            if (it.findFirstIntersection(m_current, nextIntersection)) {
                // nextUsePos[it.reg] = next use of it after start of current
                auto nextUse = it.usages().lowerBound(m_current.start());
                if (nextUse) {
                    nextUsePos[i] = std::min(nextUsePos[i], nextUse.int32());
                } else {
                    nextUsePos[i] = std::min(nextUsePos[i], it.end().int32());
                }
            }
        }
    }

    // reg = register with highest nextUsePos
    int32_t reg = 0;
    int32_t highestNextUsePos = nextUsePos[0];
    for (int32_t i = 1; i < static_cast<int32_t>(nextUsePos.size()); ++i) {
        if (nextUsePos[i] > highestNextUsePos) {
            reg = i;
            highestNextUsePos = nextUsePos[i];
        }
    }

    assert(m_current.usages().size());
    int32_t currentFirstUsage = m_current.usages().items().typedFirst().int32();

    // if first usage of current is after nextUsePos[reg] then
    if (currentFirstUsage > highestNextUsePos) {
        // all other intervals are used before current, so it is best to spill current itself
        // assign spill slot to current
        // split current before its first use position that requires a register
        m_unhandled.emplace_back(m_current.splitAt(context, currentFirstUsage));
        std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        spill(context, m_current, linearFrame);
        m_current = library::LifetimeInterval();
    } else {
        // else
        // spill intervals that currently block reg
        // current.reg = reg
        m_current.setRegisterNumber(reg);
        assert(m_active[reg]);

        // split active interval for reg at position
        auto activeSpill = m_active[reg].splitAt(context, m_current.start().int32());
        assert(!m_active[reg].isEmpty());
        // What came before the split is handled, save it.
        handled(context, m_active[reg], linearFrame);
        m_active[reg] = library::LifetimeInterval();

        // The spilled region for the active interval has to end at the next use of the interval.
        assert(!activeSpill.isEmpty());
        auto afterSpill = activeSpill.splitAt(context, highestNextUsePos);
        assert(!afterSpill.isEmpty());
        m_unhandled.emplace_back(std::move(afterSpill));
        std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        spill(context, activeSpill, linearFrame);

        // split any inactive interval for reg at the end of its lifetime hole
        auto it = m_inactive[reg].begin();
        while (it != m_inactive[reg].end()) {
            // Looking for intervals that are for actual values (instead of register reservations with kInvalidValue),
            // and that begin sometime before position, because these have been evicted from the register at position,
            // and so will need to be reprocessed after the split.
            if (it->valueNumber() && it->start().int32() < m_current.start().int32()) {
                m_unhandled.emplace_back(it->splitAt(context, m_current.start().int32()));
                std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
                handled(context, *it, linearFrame);
                *it = library::LifetimeInterval();
                it = m_inactive[reg].erase(it);
            } else {
                // make sure that current does not intersect with the fixed interval for reg
                // if current intersects with the fixed interval for reg then
                int32_t firstIntersection = 0;
                if (m_current.findFirstIntersection(*it, firstIntersection)) {
                    //   split current before this intersection
                    m_unhandled.emplace_back(m_current.splitAt(context, firstIntersection));
                    std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
                }
                ++it;
            }
        }

        m_active[reg] = m_current;
        m_current = library::LifetimeInterval();
    }
}

void RegisterAllocator::spill(ThreadContext* context, library::LifetimeInterval interval,
                              library::LinearFrame linearFrame) {
    // No spilling of register blocks.
    assert(interval.valueNumber());

    int32_t spillSlot = 0;
    // Update our active spill map in case we can re-use any spill slot no longer needed.
    for (int32_t i = 1; i < static_cast<int32_t>(m_activeSpills.size()); ++i) {
        if (m_activeSpills[i]) {
            if (m_activeSpills[i].end().int32() <= interval.start().int32()) {
                auto valueNumber = m_activeSpills[i].valueNumber().int32();
                linearFrame.valueLifetimes().typedPut(
                    valueNumber,
                    linearFrame.valueLifetimes().typedAt(valueNumber).typedAdd(context, m_activeSpills[i]));
                m_activeSpills[i] = library::LifetimeInterval();
                spillSlot = i;
            }
        } else {
            spillSlot = i;
        }
    }
    // Create a new spillSlot if needed.
    if (spillSlot == 0) {
        spillSlot = m_activeSpills.size();
        m_activeSpills.emplace_back(nullptr);
    }
    // Ensure we are reserving spill slot 0 for move cycles.
    assert(spillSlot > 0);

    // Add spill instruction to moves list.
    auto lir = linearFrame.instructions().typedAt(interval.start().int32());
    lir.moves().typedPut(context, interval.registerNumber(), library::Integer(-spillSlot));

    interval.setIsSpill(true);
    interval.setSpillSlot(library::Integer(spillSlot));
    m_activeSpills[spillSlot] = interval;
}

void RegisterAllocator::handled(ThreadContext* context, library::LifetimeInterval interval,
                                library::LinearFrame linearFrame) {
    assert(!interval.isSpill());
    assert(!interval.isEmpty());

    // Check if previous lifetime was a spill, to issue unspill commands if needed.
    auto intervalValue = interval.valueNumber().int32();
    if (linearFrame.valueLifetimes().typedAt(intervalValue).size()) {
        auto lastInterval = linearFrame.valueLifetimes().typedAt(intervalValue).typedLast();
        if (lastInterval.isSpill()) {
            auto lir = linearFrame.instructions().typedAt(interval.start().int32());
            lir.moves().typedPut(context, library::Integer(-lastInterval.spillSlot().int32()),
                                 interval.registerNumber());
        }
    }

    // Update map at every hir for this value number.
    for (int32_t i = interval.start().int32(); i < interval.end().int32(); ++i) {
        // No need to add register locations for the guard intervals at the end of the program.
        if (i >= linearFrame.instructions().size()) {
            break;
        }
        if (interval.covers(i)) {
            auto lir = linearFrame.instructions().typedAt(i);
            lir.locations().typedPut(context, library::Integer(intervalValue), interval.registerNumber());
        }
    }

    // Preserve the interval in the valueLifetimes array.
    linearFrame.valueLifetimes().typedPut(
        intervalValue, linearFrame.valueLifetimes().typedAt(intervalValue).typedAdd(context, interval));
}

} // namespace hadron