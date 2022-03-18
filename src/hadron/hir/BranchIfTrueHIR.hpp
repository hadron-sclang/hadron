#ifndef SRC_HADRON_HIR_BRANCH_IF_TRUE_HIR_HPP_
#define SRC_HADRON_HIR_BRANCH_IF_TRUE_HIR_HPP_

#include "hadron/Block.hpp"
#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct BranchIfTrueHIR : public HIR {
    BranchIfTrueHIR() = delete;
    explicit BranchIfTrueHIR(ID cond);
    virtual ~BranchIfTrueHIR() = default;

    ID condition;
    Block::ID blockId;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_BRANCH_IF_TRUE_HIR_HPP_