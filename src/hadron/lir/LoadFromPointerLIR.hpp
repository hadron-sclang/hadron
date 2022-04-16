#ifndef SRC_HADRON_LIR_LOAD_FROM_POINTER_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_FROM_POINTER_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct LoadFromPointerLIR : public LIR {
    LoadFromPointerLIR() = delete;
    LoadFromPointerLIR(VReg p, int32_t off):
        LIR(kLoadFromPointer, TypeFlags::kAllFlags),
        pointer(p),
        offset(off) { read(pointer); }
    virtual ~LoadFromPointerLIR() = default;

    VReg pointer;
    int32_t offset;

    bool producesValue() const override { return true; }

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->ldxi_w(locate(value), locate(pointer), offset * kSlotSize);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_FROM_POINTER_LIR_HPP_