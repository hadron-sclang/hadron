#ifndef SRC_HADRON_HIR_BRANCH_HIR_HPP_
#define SRC_HADRON_HIR_BRANCH_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct BranchHIR : public HIR {
    BranchHIR();
    virtual ~BranchHIR() = default;

    int32_t blockNumber;
    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_BRANCH_HIR_HPP_