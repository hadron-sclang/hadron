#ifndef SRC_HADRON_LIR_LOAD_FROM_FRAME_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_FROM_FRAME_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct LoadFromFrameLIR : public LIR {
    LoadFromFrameLIR() = delete;
    LoadFromFrameLIR(int32_t off):
        LIR(kLoadFromFrame, TypeFlags::kAllFlags),
        offset(off) { }
    virtual ~LoadFromFrameLIR() = default;

    int32_t offset;

    bool producesValue() const override { return true; }

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->ldxi_w(locations.at(value), JIT::kFramePointerReg, offset);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_FROM_FRAME_LIR_HPP_