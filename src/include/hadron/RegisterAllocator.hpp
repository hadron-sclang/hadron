#ifndef SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_
#define SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_

namespace hadron {

struct LifetimeInterval;
struct LinearBlock;

class RegisterAllocator {
public:
    RegisterAllocator() = default;
    ~RegisterAllocator() = default;

    void allocateRegisters(LinearBlock* linearBlock);
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_