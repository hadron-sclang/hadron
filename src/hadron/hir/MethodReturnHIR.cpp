#include "hadron/hir/MethodReturnHIR.hpp"

#include "hadron/lir/LoadFramePointerLIR.hpp"

namespace hadron {
namespace hir {

MethodReturnHIR::MethodReturnHIR(NVID retVal):
        HIR(kMethodReturn), returnValue(retVal) {
    reads.emplace(retVal);
}

NVID MethodReturnHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

void MethodReturnHIR::lower(const std::vector<HIR*>& values, std::vector<lir::LIR*>& vRegs,
        std::list<std::unique_ptr<lir::LIR>>& append) const {
    // Load the Frame Pointer into a virtual register.
    append.emplace_back(std::make_unique<lir::LoadFramePointerLIR>(static_cast<lir::VReg>(vRegs.size())));
    // should these be list iterators too? So you can get all swapsies with them?
    vRegs.emplace_back(append.back().get());
    // Store the function return value at

    // Something like:
    //    LoadFramePointer (which requires minting a new vReg)
    //    StoreToPointer
    //    LoadFromPointer (to load the return address) <q: is this boxed?>
    //    JumpToPointer
}

} // namespace hir
} // namespace hadron