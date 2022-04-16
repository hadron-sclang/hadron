#ifndef SRC_HADRON_LIR_BRANCH_TO_REGISTER_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_TO_REGISTER_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchToRegisterLIR : public LIR {
    BranchToRegisterLIR() = delete;
    explicit BranchToRegisterLIR(VReg a):
        LIR(kBranchToRegister, TypeFlags::kNoFlags),
        address(a) { read(address); }
    virtual ~BranchToRegisterLIR() = default;

    VReg address;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
        jit->jmpr(locate(address));
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_BRANCH_TO_REGISTER_LIR_HPP_