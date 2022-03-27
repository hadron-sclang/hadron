#include "hadron/hir/ReadFromThisHIR.hpp"

namespace hadron {
namespace hir {

ReadFromThisHIR::ReadFromThisHIR(hir::ID tID, int32_t idx, library::Symbol name):
    HIR(kReadFromThisHIR, TypeFlags::kAllFlags),
    thisId(tID),
    index(idx),
    valueName(name) { reads.emplace(tID); }

ID ReadFromThisHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ReadFromThisHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        assert(thisId == original);
        thisId = replacement;
        return true;
    }

    return false;
}

void ReadFromThisHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron
