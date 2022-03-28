#include "hadron/hir/LoadOuterFrameHIR.hpp"

namespace hadron {
namespace hir {

LoadOuterFrameHIR::LoadOuterFrameHIR(hir::ID inner):
    HIR(kLoadOuterFrame, TypeFlags::kObject), innerContext(inner) {}

ID LoadOuterFrameHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool LoadOuterFrameHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void LoadOuterFrameHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron