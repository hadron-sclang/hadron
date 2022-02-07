#ifndef SRC_HADRON_HIR_BRANCH_HIR_HPP_
#define SRC_HADRON_HIR_BRANCH_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct BranchHIR : public HIR {
    BranchHIR();
    virtual ~BranchHIR() = default;

    int blockNumber;
    NVID proposeValue(NVID id) override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_BRANCH_HIR_HPP_