#ifndef SRC_HADRON_LIR_LABEL_LIR_HPP_
#define SRC_HADRON_LIR_LABEL_LIR_HPP_

#include "hadron/Block.hpp"
#include "hadron/lir/LIR.hpp"
#include "hadron/lir/PhiLIR.hpp"

#include <vector>

namespace hadron {
namespace lir {

struct LabelLIR : public LIR {
    LabelLIR() = delete;
    LabelLIR(Block::ID block): LIR(kLabel, kInvalidVReg, Type::kNone), blockId(block) {}
    virtual ~LabelLIR() = default;

    Block::ID blockId;
    std::vector<Block::ID> predecessors;
    std::vector<Block::ID> successors;

    std::vector<std::unique_ptr<PhiLIR>> phis;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LABEL_LIR_HPP_