#ifndef SRC_HADRON_LIR_BRANCH_LIR_HPP_
#define SRC_HADRON_LIR_BRANCH_LIR_HPP_

#include "hadron/Block.hpp"
#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct BranchLIR : public LIR {
    BranchLIR() = delete;
    explicit BranchLIR(Block::ID block): LIR(kBranch, kInvalidVReg, Type::kNone), blockId(block) {}
    virtual ~BranchLIR() = default;

    Block::ID blockId;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_BRANCH_LIR_HPP_