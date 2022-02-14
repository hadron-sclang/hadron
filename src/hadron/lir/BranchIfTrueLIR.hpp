#ifndef SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_IF_TRUE_LIR_HPP_

#include "hadron/Block.hpp"
#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchIfTrueLIR : public LIR {
    BranchIfTrueLIR() = delete;
    explicit BranchIfTrueLIR(VReg cond, Block::ID block):
            LIR(kBranchIfTrue, kInvalidVReg, Type::kNone),
            condition(cond),
            blockId(block) { reads.emplace(condition); }
    virtual ~BranchIfTrueLIR() = default;

    VReg condition;
    Block::ID blockId;
};

} // namespace lir
} // namespace hadron

#endif