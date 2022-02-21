#ifndef SRC_HADRON_LIR_PHI_LIR_HPP_
#define SRC_HADRON_LIR_PHI_LIR_HPP_

#include "hadron/lir/LIR.hpp"

#include <cassert>

namespace hadron {
namespace lir {

struct PhiLIR : public LIR {
    PhiLIR() = delete;
    explicit PhiLIR(VReg v): LIR(kPhi, v, TypeFlags::kNoFlags) {}
    virtual ~PhiLIR() = default;

    std::vector<VReg> inputs;

    void addInput(VReg input, std::vector<LIRList::iterator>& vRegs) {
        assert(input != kInvalidVReg);
        reads.emplace(input);
        inputs.emplace_back(input);
        typeFlags = static_cast<TypeFlags>(static_cast<int32_t>(typeFlags) |
                                           static_cast<int32_t>((*vRegs[input])->typeFlags));
    }

    void emit(JIT* /* jit */, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        assert(false); // phis are internal language constructs and should not be emitted.
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_PHI_LIR_HPP_