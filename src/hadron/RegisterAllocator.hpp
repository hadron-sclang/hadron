#ifndef SRC_COMPILER_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_

#include "hadron/library/HadronLifetimeInterval.hpp"
#include "hadron/library/HadronLinearFrame.hpp"

#include <list>
#include <vector>

namespace hadron {

struct ThreadContext;

// The RegisterAllocator takes a LinearFrame in SSA form with lifetime ranges and outputs a register allocation
// schedule for each value.
//
// This class implements the Linear Scan algorithm detailed in [RA4] in the bibliography, "Optimized Interval Splitting
// in a Linear Scan Register Allocator", by C. Wimmer and H. Mössenböck, including the modifications to the algorithm to
// accomodate SSA form suggested in [RA5], "Linear Scan Register Allocation on SSA Form", by C. Wimmer and M. Franz.
class RegisterAllocator {
public:
    RegisterAllocator() = delete;
    RegisterAllocator(int32_t numberOfRegisters);
    ~RegisterAllocator() = default;

    void allocateRegisters(ThreadContext* context, library::LinearFrame linearFrame);

private:
    bool tryAllocateFreeReg(ThreadContext* context);
    void allocateBlockedReg(ThreadContext* context, library::LinearFrame linearFrame);
    void spill(ThreadContext* context, library::LifetimeInterval interval, library::LinearFrame linearFrame);
    void handled(ThreadContext* context, library::LifetimeInterval interval, library::LinearFrame linearFrame);

    library::LifetimeInterval m_current;
    std::vector<library::LifetimeInterval> m_unhandled;
    std::vector<library::LifetimeInterval> m_active;
    std::vector<std::list<library::LifetimeInterval>> m_inactive;
    std::vector<library::LifetimeInterval> m_activeSpills;
    int32_t m_numberOfRegisters;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_