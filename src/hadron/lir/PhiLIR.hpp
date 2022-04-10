#ifndef SRC_HADRON_LIR_PHI_LIR_HPP_
#define SRC_HADRON_LIR_PHI_LIR_HPP_

#include "hadron/lir/LIR.hpp"

#include <cassert>

namespace hadron {
namespace lir {

struct PhiLIR : public LIR {
    explicit PhiLIR(): LIR(kPhi, TypeFlags::kNoFlags) {}
    virtual ~PhiLIR() = default;

    std::vector<VReg> inputs;

    void addInput(LIR* input) {
        assert(input->value != kInvalidVReg);
        reads.emplace(input->value);
        inputs.emplace_back(input->value);
        typeFlags = static_cast<TypeFlags>(static_cast<int32_t>(typeFlags) |
                                           static_cast<int32_t>(lir->typeFlags));
    }

    void emit(JIT* /* jit */, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        assert(false); // phis are internal language constructs and should not be emitted.
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_PHI_LIR_HPP_