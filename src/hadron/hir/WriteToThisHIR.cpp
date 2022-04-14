#include "hadron/hir/WriteToThisHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/StoreToPointerLIR.hpp"

namespace hadron {
namespace hir {

WriteToThisHIR::WriteToThisHIR(hir::ID tID, int32_t idx, library::Symbol name, hir::ID write):
        HIR(kWriteToThis),
        thisId(tID),
        index(idx),
        valueName(name),
        toWrite(write) {
    reads.emplace(thisId);
    reads.emplace(toWrite);
}

ID WriteToThisHIR::proposeValue(ID /* proposedId */) {
    return kInvalidID;
}

bool WriteToThisHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        if (thisId == original) {
            thisId = replacement;
        }
        if (toWrite == original) {
            toWrite = replacement;
        }
        return true;
    }

    return false;
}

void WriteToThisHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* linearFrame) const {
    auto thisVReg = linearFrame->hirToReg(thisId);
    auto toStoreVReg = linearFrame->hirToReg(toWrite);
    linearFrame->append(kInvalidID, std::make_unique<lir::StoreToPointerLIR>(thisVReg, toStoreVReg, index));
}

} // namespace hir
} // namespace hadron
