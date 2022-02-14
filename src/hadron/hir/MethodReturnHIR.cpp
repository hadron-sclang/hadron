#include "hadron/hir/MethodReturnHIR.hpp"

#include "hadron/lir/BranchToRegisterLIR.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"
#include "hadron/lir/StoreToPointerLIR.hpp"

namespace hadron {
namespace hir {

MethodReturnHIR::MethodReturnHIR(NVID retVal):
        HIR(kMethodReturn), returnValue(retVal) {
    reads.emplace(retVal);
}

NVID MethodReturnHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

void MethodReturnHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    // Store the function return value to the frame pointer + 0.
    lir::VReg framePointer = append.back()->value;
    append.emplace_back(std::make_unique<lir::StoreToPointerLIR>(framePointer, values[returnValue]->vReg(), 0));

    // Load the return address from the frame pointer + 1.
    append.emplace_back(std::make_unique<lir::LoadFromPointerLIR>(static_cast<lir::VReg>(vRegs.size()), framePointer,
            1));
    vRegs.emplace_back(--append.end());

    // Jump to the return address.
    append.emplace_back(std::make_unique<lir::BranchToRegisterLIR>(append.back()->value));
}

} // namespace hir
} // namespace hadron