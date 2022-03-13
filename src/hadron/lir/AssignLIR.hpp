#ifndef SRC_HADRON_LIR_ASSIGN_LIR_HPP_
#define SRC_HADRON_LIR_ASSIGN_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct AssignLIR : public LIR {
    AssignLIR() = delete;
    explicit AssignLIR(VReg dest, VReg orig, TypeFlags originType):
        LIR(kAssign, dest, originType), origin(orig) { reads.emplace(origin); }
    virtual ~AssignLIR() = default;

    VReg origin;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->movr(locations.at(value), locations.at(origin));
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_ASSIGN_LIR_HPP_