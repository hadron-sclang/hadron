#include "hadron/hir/LoadOuterFrameHIR.hpp"

namespace hadron {
namespace hir {

LoadOuterFrameHIR::LoadOuterFrameHIR(hir::ID inner):
    HIR(kLoadOuterFrame, TypeFlags::kObjectFlag), innerContext(inner) {}

ID LoadOuterFrameHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool LoadOuterFrameHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void LoadOuterFrameHIR::lower(LinearFrame* /* linearFrame */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron