#include "hadron/hir/MethodReturnHIR.hpp"

#include "hadron/library/Kernel.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/BranchToRegisterLIR.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"

namespace hadron {
namespace hir {

MethodReturnHIR::MethodReturnHIR():
        HIR(kMethodReturn) {}

ID MethodReturnHIR::proposeValue(ID /* proposedId */) {
    return kInvalidID;
}

bool MethodReturnHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void MethodReturnHIR::lower(LinearFrame* linearFrame) const {
    // Load caller Frame pointer.
    auto callerFrame = linearFrame->append(kInvalidID, std::make_unique<lir::LoadFromPointerLIR>(lir::kFramePointerVReg,
            offsetof(schema::FramePrivateSchema, caller)));
    // Load return address into a register value
    auto returnAddress = linearFrame->append(kInvalidID,
            std::make_unique<lir::LoadFromPointerLIR>(callerFrame, offsetof(schema::FramePrivateSchema, ip)));
    // Branch to that register value
    linearFrame->append(kInvalidID, std::make_unique<lir::BranchToRegisterLIR>(returnAddress));
}

} // namespace hir
} // namespace hadron