#ifndef SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_
#define SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_

namespace hadron {

struct LinearBlock;

class RegisterAllocator {
public:
    RegisterAllocator() = default;
    ~RegisterAllocator() = default;

    void allocateRegisters(LinearBlock* linearBlock, size_t numberOfRegisters);
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_REGISTER_ALLOCATOR_HPP_