#ifndef SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchIfTrueLIR : public LIR {
    BranchIfTrueLIR() = delete;
    explicit BranchIfTrueLIR(VReg cond, LabelID label):
            LIR(kBranchIfTrue, kInvalidVReg, Type::kNone),
            condition(cond),
            labelId(label) { reads.emplace(condition); }
    virtual ~BranchIfTrueLIR() = default;

    VReg condition;
    LabelID labelId;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& patchNeeded) const override {
        emitBase(jit);
        patchNeeded.emplace_back(std::make_pair(jit->beqi(locations.at(condition), 1), labelId));
    }
};

} // namespace lir
} // namespace hadron

#endif