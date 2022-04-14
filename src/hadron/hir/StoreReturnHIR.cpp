#include "hadron/hir/StoreReturnHIR.hpp"

#include "hadron/library/Kernel.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/lir/StoreToFrameLIR.hpp"

namespace hadron {
namespace hir {

StoreReturnHIR::StoreReturnHIR(ID value):
    HIR(kStoreReturn),
    returnValue(value) { reads.emplace(returnValue); }

ID StoreReturnHIR::proposeValue(ID /* proposedId */) {
    return kInvalidID;
}

bool StoreReturnHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        assert(returnValue == original);
        returnValue = replacement;
        return true;
    }

    return false;
}

void StoreReturnHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* linearFrame) const {
    // Overwrite the value at argument 0 with the return value.
    auto returnValueVReg = linearFrame->hirToReg(returnValue);
    linearFrame->append(hir::kInvalidID, std::make_unique<lir::StoreToFrameLIR>(returnValueVReg,
            static_cast<int32_t>(sizeof(schema::FramePrivateSchema))));
}

} // namespace hir
} // namespace hadron