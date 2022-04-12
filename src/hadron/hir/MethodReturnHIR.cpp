#include "hadron/hir/MethodReturnHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/BranchToRegisterLIR.hpp"
#include "hadron/lir/LoadFromStackLIR.hpp"

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

void MethodReturnHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* linearFrame) const {
    // Load return address into a register value
    auto returnAddress = linearFrame->append(kInvalidID, std::make_unique<lir::LoadFromStackLIR>(true, 2));
    // Branch to that register value
    linearFrame->append(kInvalidID, std::make_unique<lir::BranchToRegisterLIR>(returnAddress));
}

} // namespace hir
} // namespace hadron