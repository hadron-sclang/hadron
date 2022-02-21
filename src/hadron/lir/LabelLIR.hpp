#ifndef SRC_HADRON_LIR_LABEL_LIR_HPP_
#define SRC_HADRON_LIR_LABEL_LIR_HPP_

#include "hadron/lir/LIR.hpp"
#include "hadron/lir/PhiLIR.hpp"

#include <vector>

namespace hadron {
namespace lir {

struct LabelLIR : public LIR {
    LabelLIR() = delete;
    explicit LabelLIR(LabelID labelId): LIR(kLabel, kInvalidVReg, TypeFlags::kNoFlags), id(labelId) {}
    virtual ~LabelLIR() = default;

    LabelID id;
    std::vector<LabelID> predecessors;
    std::vector<LabelID> successors;

    LIRList phis;

    void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& /* patchNeeded */) const override {
        emitBase(jit);
    }
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_LABEL_LIR_HPP_