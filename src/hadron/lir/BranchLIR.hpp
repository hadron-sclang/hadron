#ifndef SRC_HADRON_LIR_BRANCH_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_LIR_HPP_

#include "hadron/Block.hpp"
#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchLIR : public LIR {
    BranchLIR() = delete;
    explicit BranchLIR(LabelID label): LIR(kBranch, kInvalidVReg, TypeFlags::kNoFlags), labelId(label) {}
    virtual ~BranchLIR() = default;

    LabelID labelId;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& patchNeeded) const override {
        emitBase(jit);
        patchNeeded.emplace_back(std::make_pair(jit->jmp(), labelId));
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_BRANCH_LIR_HPP_