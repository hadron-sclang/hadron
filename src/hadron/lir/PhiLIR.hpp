#ifndef SRC_HADRON_LIR_PHI_LIR_HPP_
#define SRC_HADRON_LIR_PHI_LIR_HPP_

#include "hadron/lir/LIR.hpp"

namespace hadron {
namespace lir {

struct PhiLIR : public LIR {
    PhiLIR() = delete;
    explicit PhiLIR(VReg v): LIR(kPhi, v) {}
    virtual ~PhiLIR() = default;

    std::vector<VReg> inputs;

    void addInput(VReg input) {
        reads.emplace(input);
        inputs.emplace_back(input);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_PHI_LIR_HPP_