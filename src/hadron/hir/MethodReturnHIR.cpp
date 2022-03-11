#include "hadron/hir/MethodReturnHIR.hpp"

#include "hadron/lir/BranchToRegisterLIR.hpp"
#include "hadron/lir/LoadFromStackLIR.hpp"

namespace hadron {
namespace hir {

MethodReturnHIR::MethodReturnHIR():
        HIR(kMethodReturn) { }

NVID MethodReturnHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

bool MethodReturnHIR::replaceInput(NVID /* original */, NVID /* replacement */) {
    return false;
}

void MethodReturnHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    // Load return address into a register value
    auto returnAddress = static_cast<lir::VReg>(vRegs.size());
    append.emplace_back(std::make_unique<lir::LoadFromStackLIR>(returnAddress, true, 2));
    vRegs.emplace_back(--(append.end()));
    // Branch to that register value
    append.emplace_back(std::make_unique<lir::BranchToRegisterLIR>(returnAddress));
}

} // namespace hir
} // namespace hadron