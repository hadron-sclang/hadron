#ifndef SRC_HADRON_LIR_LABEL_LIR_HPP_
#define SRC_HADRON_LIR_LABEL_LIR_HPP_

#include "hadron/lir/LIR.hpp"
#include "hadron/lir/PhiLIR.hpp"

#include <vector>

namespace hadron {
namespace lir {

struct LabelLIR : public LIR {
    LabelLIR() = delete;
    LabelLIR(int32_t blockNum): LIR(kLabel, kInvalidVReg, Type::kNone), blockNumber(blockNum) {}
    virtual ~LabelLIR() = default;

    int32_t blockNumber;
    std::vector<int32_t> predecessors;
    std::vector<int32_t> successors;

    std::vector<std::unique_ptr<PhiLIR>> phis;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LABEL_LIR_HPP_