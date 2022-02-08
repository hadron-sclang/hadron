#ifndef SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchIfTrueLIR : public LIR {
    BranchIfTrueLIR() = delete;
    explicit BranchIfTrueLIR(VReg cond, int32_t blockNum):
            LIR(kBranchIfTrue),
            condition(cond),
            blockNumber(blockNum) { arguments.emplace_back(condition); }
    virtual ~BranchIfTrueLIR() = default;

    VReg condition;
    int32_t blockNumber;
};

} // namespace lir
} // namespace hadron

#endif