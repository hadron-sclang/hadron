#ifndef SRC_HADRON_LIR_LOAD_FROM_STACK_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_FROM_STACK_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct LoadFromStackLIR : public LIR {
    LoadFromStackLIR() = delete;
    LoadFromStackLIR(bool useFP, int32_t off):
            LIR(kLoadFromStack, TypeFlags::kAllFlags),
            useFramePointer(useFP),
            offset(off) {}
    virtual ~LoadFromStackLIR() = default;

    bool useFramePointer;
    int32_t offset;

    bool producesValue() const override { return true; }

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->ldxi_w(locations.at(value), useFramePointer ? JIT::kFramePointerReg : JIT::kStackPointerReg,
            offset * kSlotSize);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_FROM_STACK_LIR_HPP_