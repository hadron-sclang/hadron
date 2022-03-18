#include "hadron/hir/StoreReturnHIR.hpp"

#include "hadron/lir/StoreToStackLIR.hpp"

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

void StoreReturnHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& append) const {
    // Overwrite the value at argument 0 with the return value.
    append.emplace_back(std::make_unique<lir::StoreToStackLIR>(values[returnValue]->vReg(), false, -1));
}

} // namespace hir
} // namespace hadron