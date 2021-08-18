#ifndef SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_
#define SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_

#include <list>
#include <vector>

namespace hadron {

struct LinearBlock;
struct RegInterval;

class RegisterAllocator {
public:
    RegisterAllocator() = default;
    ~RegisterAllocator() = default;

    void allocateRegisters(LinearBlock* linearBlock);

private:
    std::vector<RegInterval> m_unhandled;
    std::vector<std::list<RegInterval>::iterator> m_inactive;
    std::vector<std::list<RegInterval>::iterator> m_active;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_