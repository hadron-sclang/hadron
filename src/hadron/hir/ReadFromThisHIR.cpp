#include "hadron/hir/ReadFromThisHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"

namespace hadron {
namespace hir {

ReadFromThisHIR::ReadFromThisHIR(hir::ID tID, int32_t idx, library::Symbol name):
    HIR(kReadFromThis, TypeFlags::kAllFlags),
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

void ReadFromThisHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* linearFrame) const {
    auto thisIdVReg = linearFrame->hirToReg(thisId);
    linearFrame->append(id, std::make_unique<lir::LoadFromPointerLIR>(thisIdVReg, index));
}

} // namespace hir
} // namespace hadron
