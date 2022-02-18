#ifndef SRC_HADRON_LIR_LOAD_CONSTANT_LIR_HPP_
#define SRC_HADRON_LIR_LOAD_CONSTANT_LIR_HPP_

#include "hadron/lir/LIR.hpp"
#include "hadron/Slot.hpp"

namespace hadron {
namespace lir {

struct LoadConstantLIR : public LIR {
    LoadConstantLIR() = delete;
    LoadConstantLIR(VReg v, Slot c):
        LIR(kLoadConstant, v, c.getType()),
        constant(c) {}
    virtual ~LoadConstantLIR() = default;

    Slot constant;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->movi_u(locations.at(value), constant.asBits());
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LOAD_CONSTANT_LIR_HPP_