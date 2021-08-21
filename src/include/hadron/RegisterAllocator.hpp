#ifndef SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_
#define SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

struct LifetimeInterval;
struct LinearBlock;

// The RegisterAllocator takes a LinearBlock in SSA form with lifetime ranges and outputs a register allocation
// schedule for each value.
//
// This class implements the Linear Scan algorithm detailed in [RA4] in the bibliography, "Optimized Interval Splitting
// in a Linear Scan Register Allocator", by C. Wimmer and H. Mössenböck, including the modifications to the algorithm to
// accomodate SSA form suggested in [RA5], "Linear Scan Register Allocation on SSA Form", by C. Wimmer and M. Franz.
class RegisterAllocator {
public:
    RegisterAllocator() = default;
    ~RegisterAllocator() = default;

    void allocateRegisters(LinearBlock* linearBlock);

private:
    bool tryAllocateFreeReg(LifetimeInterval& current);
    void allocateBlockedReg(LifetimeInterval& current, LinearBlock* linearBlock);
    void spill(LifetimeInterval& interval, LinearBlock* linearBlock);

    std::vector<LifetimeInterval> m_unhandled;
    std::unordered_map<size_t, LifetimeInterval> m_active;
    std::unordered_map<size_t, LifetimeInterval> m_inactive;
    std::unordered_map<size_t, LifetimeInterval> m_activeSpills;
    std::unordered_set<size_t> m_freeSpills;
    size_t m_numberOfRegisters = 0;
    size_t m_numberOfSpillSlots = 0;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_