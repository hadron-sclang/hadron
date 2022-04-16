#include "hadron/hir/LoadOuterFrameHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"

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

void LoadOuterFrameHIR::lower(LinearFrame* linearFrame) const {
    auto innerContextVReg = innerContext == hir::kInvalidID ? lir::kFramePointerVReg
            : linearFrame->hirToReg(innerContext);
    linearFrame->append(id, std::make_unique<lir::LoadFromPointerLIR>(innerContextVReg,
            offsetof(schema::FramePrivateSchema, context)));
}

} // namespace hir
} // namespace hadron