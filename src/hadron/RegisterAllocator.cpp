#include "hadron/RegisterAllocator.hpp"

#include "hadron/HIR.hpp"
#include "hadron/LinearBlock.hpp"

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
    bool operator()(const hadron::LtIRef& lt1, const hadron::LtIRef& lt2) const {
        return (lt1->start() > lt2->start());
    }
};
} // namespace

namespace hadron {

RegisterAllocator::RegisterAllocator(size_t numberOfRegisters): m_numberOfRegisters(numberOfRegisters) {
    m_active.resize(m_numberOfRegisters);
    m_inactive.resize(m_numberOfRegisters);
}

void RegisterAllocator::allocateRegisters(LinearBlock* linearBlock) {
    // We build a min-heap of nonemtpy value lifetimes, ordered by start time. Higher-number values are likely to start
    // later in the block, so we add them to the heap in reverse order.
    m_unhandled.reserve(linearBlock->valueLifetimes.size());
    for (int i = linearBlock->valueLifetimes.size() - 1; i >= 0; --i) {
        if (!linearBlock->valueLifetimes[i][0]->isEmpty()) {
            m_unhandled.emplace_back(std::move(linearBlock->valueLifetimes[i][0]));
            linearBlock->valueLifetimes[i].clear();
        }
    }
    std::make_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());

    // unhandled = list of intervals sorted by increasing start positions
    // active = { }; inactive = { }; handled = { };

    // Populate m_inactive with any register reservations, and add at least one usage for every register at the end
    // of the program, useful for minimizing corner cases in calculation of register next usage during allocation.
    size_t numberOfInstructions = linearBlock->instructions.size();
    for (size_t i = 0; i < m_numberOfRegisters; ++i) {
        auto regLifetime = std::make_unique<LifetimeInterval>();
        regLifetime->registerNumber = i;
        regLifetime->usages.emplace(numberOfInstructions);
        regLifetime->addLiveRange(numberOfInstructions, numberOfInstructions + 1);
        m_inactive[i].emplace_back(std::move(regLifetime));
    }
    // Iterate through all instructions and add additional reservations as needed.
    for (size_t i = 0; i < numberOfInstructions; ++i) {
        int reservedRegisters = linearBlock->instructions[i]->numberOfReservedRegisters();
        if (reservedRegisters < 0) { reservedRegisters = m_numberOfRegisters; }
        SPDLOG_DEBUG("Reserving {} registers at instruction {}", reservedRegisters, i);
        // A HIR cannot reserve and read more registers than are available on the machine. This is a sign of a flaw in
        // the HIR design, and probably means the HIR needs to be broken out to more instructions that can handle the
        // values separately. Register allocation will fail at any instruction requiring more registers than available.
        assert(reservedRegisters + static_cast<int>(linearBlock->instructions[i]->reads.size()) <=
                static_cast<int>(m_numberOfRegisters));
        for (int j = 0; j < reservedRegisters; ++j) {
            auto regLifetime = m_inactive[m_numberOfRegisters - j - 1].back().get();
            regLifetime->addLiveRange(i, i + 1);
            regLifetime->usages.emplace(i);
        }
    }

    m_activeSpills.resize(linearBlock->numberOfSpillSlots);

    // while unhandled =/= { } do
    while (m_unhandled.size()) {
        // current = pick and remove first interval from unhandled
        std::pop_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        m_current = std::move(m_unhandled.back());
        SPDLOG_DEBUG("current interval value: {} start: {} end: {}, with {} ranges and {} usages.",
                m_current->valueNumber, m_current->start(), m_current->end(), m_current->ranges.size(),
                m_current->usages.size());
        m_unhandled.pop_back();
        assert(!m_current->isEmpty());

        // position = start position of current
        size_t position = m_current->start();

        // check for intervals in active that are handled or inactive
        // for each interval it in active do
        for (size_t reg = 0; reg < m_active.size(); ++reg) {
            if (!m_active[reg]) { continue; }
            // if it ends before position then
            if (m_active[reg]->end() <= position) {
                // move it from active to handled
                SPDLOG_DEBUG("* at position {} moving value {} from active to handled", position,
                        m_active[reg]->valueNumber);
                handled(std::move(m_active[reg]), linearBlock);
                m_active[reg] = nullptr;
            } else {
                // else if it does not cover position then
                if (!m_active[reg]->covers(position)) {
                    // move it from active to inactive
                    m_inactive[reg].emplace_back(std::move(m_active[reg]));
                    m_active[reg] = nullptr;
                }
            }
        }

        // check for intervals in inactive that are handled or active
        // for each interval it in inactive do
        for (size_t reg = 0; reg < m_inactive.size(); ++reg) {
            auto iter = m_inactive[reg].begin();
            while (iter != m_inactive[reg].end()) {
                // if it ends before position then
                if ((*iter)->end() <= position) {
                    // move it from inactive to handled
                    SPDLOG_DEBUG("* at position {} moving value {} from inactive to handled", position,
                            (*iter)->valueNumber);
                    handled(std::move(*iter), linearBlock);
                    iter = m_inactive[reg].erase(iter);
                } else {
                    // else if it covers position then
                    if ((*iter)->covers(position)) {
                        assert(m_active[reg] == nullptr);
                        m_active[reg] = std::move(*iter);
                        iter = m_inactive[reg].erase(iter);
                    } else { ++iter; }
                }
            }
        }

        // find a register for current
        // TRYALLOCATEFREEREG
        if (!tryAllocateFreeReg()) {
            // if allocation failed then ALLOCATEBLOCKEDREG
            allocateBlockedReg(linearBlock);
        }
    }

    // Append any final lifetimes to the linearBlock.
    for (size_t reg = 0; reg < m_active.size(); ++reg) {
        if (m_active[reg]) {
            if (m_active[reg]->valueNumber != std::numeric_limits<uint32_t>::max()) {
                handled(std::move(m_active[reg]), linearBlock);
            }
            m_active[reg] = nullptr;
        }
        for (auto& iter : m_inactive[reg]) {
            if (iter->valueNumber != std::numeric_limits<uint32_t>::max()) {
                handled(std::move(iter), linearBlock);
            }
        }
        m_inactive[reg].clear();
    }
    for (size_t spill = 1; spill < m_activeSpills.size(); ++spill) {
        if (m_activeSpills[spill] != nullptr) {
            linearBlock->valueLifetimes[m_activeSpills[spill]->valueNumber].emplace_back(
                    std::move(m_activeSpills[spill]));
            m_activeSpills[spill] = nullptr;
        }
    }
    linearBlock->numberOfSpillSlots = m_activeSpills.size();
}

bool RegisterAllocator::tryAllocateFreeReg() {
    // set freeUntilPos of all physical registers to maxInt
    std::vector<size_t> freeUntilPos(m_numberOfRegisters, std::numeric_limits<size_t>::max());

    // for each interval it in active do
    for (size_t i = 0; i < m_numberOfRegisters; ++i) {
        if (m_active[i]) {
            // freeUntilPos[it.reg] = 0
            freeUntilPos[i] = 0;
        } else {
            // for each interval it in inactive intersecting with current do
            for (auto& it : m_inactive[i]) {
                size_t nextIntersection = 0;
                if (it->findFirstIntersection(m_current.get(), nextIntersection)) {
                    // freeUntilPos[it.reg] = next intersection of it with current
                    freeUntilPos[i] = std::min(freeUntilPos[i], nextIntersection);
                }
            }
        }
    }

    // reg = register with highest freeUntilPos
    size_t reg = 0;
    size_t highestFreeUntilPos = freeUntilPos[0];
    for (size_t i = 0; i < freeUntilPos.size(); ++i) {
        if (freeUntilPos[i] > highestFreeUntilPos) {
            reg = i;
            highestFreeUntilPos = freeUntilPos[i];
        }
    }

    SPDLOG_DEBUG("* tryAllocate register: {} freeUntilPos: {}", reg, highestFreeUntilPos);

    // if freeUntilPos[reg] = 0 then
    if (highestFreeUntilPos == 0) {
        SPDLOG_DEBUG("* tryAllocate found no register available");
        // no register available without spilling
        // allocation failed
        return false;
    } else if (m_current->end() <= highestFreeUntilPos) {
        SPDLOG_DEBUG("* tryAllocate found available register {}", reg);
        // else if current ends before freeUntilPos[reg] then
        //   // register available for the whole interval
        //   current.reg = reg
        m_current->registerNumber = reg;
    } else {
        // else
        //   // register available for the first part of the interval
        //   current.reg = reg
        m_current->registerNumber = reg;
        // split current before freeUntilPos[reg]
        m_unhandled.emplace_back(m_current->splitAt(highestFreeUntilPos));
        SPDLOG_DEBUG("* tryAllocate found split, current start: {} end: {}, unhandled start: {}, end: {}",
                m_current->start(), m_current->end(), m_unhandled.back()->start(), m_unhandled.back()->end());
        std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
    }

    assert(m_active[reg] == nullptr);
    m_active[reg] = std::move(m_current);
    m_current = nullptr;
    return true;
}

void RegisterAllocator::allocateBlockedReg(LinearBlock* linearBlock) {
    // set nextUsePos of all physical registers to maxInt
    std::vector<size_t> nextUsePos(m_numberOfRegisters, std::numeric_limits<size_t>::max());

    // for each interval it in active do
    for (size_t i = 0; i < m_numberOfRegisters; ++i) {
        if (m_active[i]) {
            // nextUsePos[it.reg] = next use of it after start of current
            auto nextUse = m_active[i]->usages.upper_bound(m_current->start());
            if (nextUse != m_active[i]->usages.end()) {
                nextUsePos[i] = *nextUse;
            } else {
                // If there's not a usage but the register is marked as active, use the end of the active interval to
                // approximate the next use. Could happen at the end of a loop block, for instance.
                nextUsePos[i] = m_active[i]->end();
            }
        } else {
            // for each interval it in inactive intersecting with current do
            for (auto& it : m_inactive[i]) {
                size_t nextIntersection = 0;
                if (it->findFirstIntersection(m_current.get(), nextIntersection)) {
                    // nextUsePos[it.reg] = next use of it after start of current
                    auto nextUse = it->usages.upper_bound(m_current->start());
                    if (nextUse != it->usages.end()) {
                        nextUsePos[i] = std::min(nextUsePos[i], *nextUse);
                    } else {
                        nextUsePos[i] = std::min(nextUsePos[i], it->end());
                    }
                }
            }
        }
    }

    // reg = register with highest nextUsePos
    size_t reg = 0;
    size_t highestNextUsePos = nextUsePos[0];
    for (size_t i = 1; i < nextUsePos.size(); ++i) {
        if (nextUsePos[i] > highestNextUsePos) {
            reg = i;
            highestNextUsePos = nextUsePos[i];
        }
    }

    // if first usage of current is after nextUsePos[reg] then
    assert(m_current->usages.size());
    size_t currentFirstUsage = *(m_current->usages.begin());

    SPDLOG_DEBUG("* allocateBlocked choosing reg {} with highest next use {}, current first use {}", reg,
            highestNextUsePos, currentFirstUsage);

    if (currentFirstUsage > highestNextUsePos) {
        // all other intervals are used before current, so it is best to spill current itself
        // assign spill slot to current
        // split current before its first use position that requires a register
        m_unhandled.emplace_back(m_current->splitAt(currentFirstUsage));
        SPDLOG_DEBUG("* allocateBlocked spilling current, new start: {} end: {}, unhandled start: {}, end: {}",
                m_current->start(), m_current->end(), m_unhandled.back()->start(), m_unhandled.back()->end());
        std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
        spill(std::move(m_current), linearBlock);
        m_current = nullptr;
    } else {
        // else
        // spill intervals that currently block reg
        // current.reg = reg
        m_current->registerNumber = reg;
        if (m_active[reg]) {
            SPDLOG_DEBUG("* allocateBlocked split active interval for reg {} at {}, start: {} end: {}",
                    reg, m_current->start(), m_active[reg]->start(), m_active[reg]->end());

            // split active interval for reg at position
            auto activeSpill = m_active[reg]->splitAt(m_current->start());
            assert(!m_active[reg]->isEmpty());
            handled(std::move(m_active[reg]), linearBlock);
            m_active[reg] = std::move(m_current);
            m_current = nullptr;

            assert(!activeSpill->isEmpty());
            SPDLOG_DEBUG("* allocateBlocked splitting spilled region start: {} end: {} at {}", activeSpill->start(),
                    activeSpill->end(), highestNextUsePos);
            auto afterSpill = activeSpill->splitAt(highestNextUsePos);
            assert(!afterSpill->isEmpty());
            m_unhandled.emplace_back(std::move(afterSpill));
            std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
            spill(std::move(activeSpill), linearBlock);
        } else {
            assert(false);
        }

/* TODO:
        auto it = m_inactive[reg].begin();
        while (it != m_inactive[reg].end()) {
            // split any inactive interval for reg at the end of its lifetime hole
            auto inactiveSpill = (*it)->splitAt(m_current->start());
            assert(!inactiveSpill->isEmpty());
            SPDLOG_DEBUG("* allocateBlocked split inactive interval for {} at {}, new start: {} end: {}, unhandled "
                    "start: {} end: {}", reg, m_current->start(), (*it)->start(), (*it)->end(),
                    inactiveSpill->start(), inactiveSpill->end());

            handled(std::move(*it), linearBlock);
            // TODO: any harm in splitting here instead of at the start of the lifetime hole?
            m_unhandled.emplace_back(inactiveSpill->splitAt(highestNextUsePos));
            std::push_heap(m_unhandled.begin(), m_unhandled.end(), IntervalCompare());
            spill(std::move(inactiveSpill), linearBlock);

            m_active.emplace(std::make_pair(reg, std::move(m_current)));
            m_current = nullptr;
            m_inactive.erase(iter);
        }
*/


        // The following pseudocode fixes up a register assignment to current in the event that it collides with a
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

void RegisterAllocator::spill(LtIRef interval, LinearBlock* linearBlock) {
    SPDLOG_DEBUG("** spill interval value: {} reg: {} start: {} end: {}, with {} ranges and {} usages.",
            interval->valueNumber, interval->registerNumber, interval->start(), interval->end(),
            interval->ranges.size(), interval->usages.size());

    size_t spillSlot = 0;
    // Update our active spill map in case we can re-use any spill slot no longer needed.
    for (size_t i = 1; i < m_activeSpills.size(); ++i) {
        if (m_activeSpills[i]) {
            if (m_activeSpills[i]->end() <= interval->start()) {
                linearBlock->valueLifetimes[m_activeSpills[i]->valueNumber].emplace_back(std::move(m_activeSpills[i]));
                m_activeSpills[i] = nullptr;
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
    linearBlock->instructions[interval->start()]->moves.emplace(std::make_pair(interval->registerNumber,
        -static_cast<int>(spillSlot)));

    interval->isSpill = true;
    interval->spillSlot = spillSlot;
    m_activeSpills[spillSlot] = std::move(interval);
}

void RegisterAllocator::handled(LtIRef interval, LinearBlock* linearBlock) {
    SPDLOG_DEBUG("** handled interval value: {} reg: {} start: {} end: {}, with {} ranges and {} usages.",
            interval->valueNumber, interval->registerNumber, interval->start(), interval->end(),
            interval->ranges.size(), interval->usages.size());
    assert(!interval->isSpill);
    assert(!interval->isEmpty());

    // Check if previous lifetime was a spill, to issue unspill commands if needed.
    if (linearBlock->valueLifetimes[interval->valueNumber].size() &&
            linearBlock->valueLifetimes[interval->valueNumber].back()->isSpill) {
        linearBlock->instructions[interval->start()]->moves.emplace(std::make_pair(
            -static_cast<int>(linearBlock->valueLifetimes[interval->valueNumber].back()->spillSlot),
            interval->registerNumber));
    }

    // Update map at every hir for this value number.
    for (size_t i = interval->start(); i < interval->end(); ++i) {
        // No need to add register locations for the guard intervals at the end of the program.
        if (i >= linearBlock->instructions.size()) {
            break;
        }
        if (interval->covers(i)) {
            linearBlock->instructions[i]->valueLocations.emplace(std::make_pair(interval->valueNumber,
                static_cast<int>(interval->registerNumber)));
        }
    }

    // Preserve the interval in the valueLifetimes array.
    linearBlock->valueLifetimes[interval->valueNumber].emplace_back(std::move(interval));
}

} // namespace hadron