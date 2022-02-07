#ifndef SRC_HADRON_LIR_LABEL_LIR_HPP_
#define SRC_HADRON_LIR_LABEL_LIR_HPP_

#include "hadron/lir/LIR.hpp"

#include <list>

namespace hadron {
namespace lir {

struct PhiLIR;

struct LabelLIR : public LIR {
    LabelLIR() = delete;
    LabelLIR(int32_t blockNum): LIR(kLabel), blockNumber(blockNum) {}
    virtual ~LabelLIR() = default;

    int32_t blockNumber;
    std::vector<int32_t> predecessors;
    std::vector<int32_t> successors;

    std::vector<std::unique_ptr<PhiLIR>> phis;

    Value proposeValue(uint32_t number) override;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LABEL_LIR_HPP_