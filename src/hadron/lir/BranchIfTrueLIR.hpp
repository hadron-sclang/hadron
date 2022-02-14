#ifndef SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchIfTrueLIR : public LIR {
    BranchIfTrueLIR() = delete;
    explicit BranchIfTrueLIR(VReg cond, int32_t blockNum):
            LIR(kBranchIfTrue, kInvalidVReg, Type::kNone),
            condition(cond),
            blockNumber(blockNum) { reads.emplace(condition); }
    virtual ~BranchIfTrueLIR() = default;

    VReg condition;
    int32_t blockNumber;
};

} // namespace lir
} // namespace hadron

#endif