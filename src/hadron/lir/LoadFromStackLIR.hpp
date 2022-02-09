#ifndef SRC_HADRON_LIR_LOAD_FROM_STACK_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_FROM_STACK_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct LoadFromStackLIR : public LIR {
    LoadFromStackLIR() = delete;
    explicit LoadFromStackLIR(VReg v, int32_t off): LIR(kLoadFromStack, v), offset(off) {}
    virtual ~LoadFromStackLIR() = default;

    int32_t offset;

    void emit(JIT* jit) const override {
        LIR::emit(jit);
        jit->ldxi_l(valueLocations.at(value), JIT::kStackPointerReg, offset * kSlotSize);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_FROM_STACK_LIR_HPP_