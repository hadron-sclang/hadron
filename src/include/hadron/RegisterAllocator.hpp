#ifndef SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_
#define SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_

#include <unordered_map>
#include <vector>

namespace hadron {

struct LifetimeInterval;
struct LinearBlock;

class RegisterAllocator {
public:
    RegisterAllocator() = default;
    ~RegisterAllocator() = default;

    void allocateRegisters(LinearBlock* linearBlock);

private:
    bool tryAllocateFreeReg(LifetimeInterval& current);
    bool allocateBlockedReg(LifetimeInterval& current);

    std::vector<LifetimeInterval> m_unhandled;
    std::unordered_map<size_t, LifetimeInterval> m_active;
    std::unordered_map<size_t, LifetimeInterval> m_inactive;
    size_t m_numberOfRegisters = 0;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_