#ifndef SRC_HADRON_LIR_BRANCH_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchLIR : public LIR {
    BranchLIR() = delete;
    explicit BranchLIR(int32_t blockNum): LIR(kBranch), blockNumber(blockNum) {}
    virtual ~BranchLIR() = default;

    int32_t blockNumber;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_BRANCH_LIR_HPP_