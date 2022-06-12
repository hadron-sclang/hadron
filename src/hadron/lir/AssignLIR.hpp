#ifndef SRC_HADRON_LIR_ASSIGN_LIR_HPP_
#define SRC_HADRON_LIR_ASSIGN_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct AssignLIR : public LIR { 
    AssignLIR() = delete;
    explicit AssignLIR(VReg orig):
        LIR(kAssign, TypeFlags::kAllFlags),
        origin(orig) {
            read(origin);
    }

    VReg origin;

    bool producesValue() const override { return true; }

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->movr(locate(value), locate(origin));
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_ASSIGN_LIR_HPP_