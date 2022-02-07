#ifndef SRC_HADRON_HIR_BRANCH_IF_TRUE_HIR_HPP_
#define SRC_HADRON_HIR_BRANCH_IF_TRUE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct BranchIfTrueHIR : public HIR {
    BranchIfTrueHIR() = delete;
    explicit BranchIfTrueHIR(NVID cond);
    virtual ~BranchIfTrueHIR() = default;

    NVID condition;
    int blockNumber;

    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_BRANCH_IF_TRUE_HIR_HPP_