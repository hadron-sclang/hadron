#include "hadron/hir/BranchIfTrueHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/BranchIfTrueLIR.hpp"

namespace hadron {
namespace hir {

BranchIfTrueHIR::BranchIfTrueHIR(ID cond): HIR(kBranchIfTrue), condition(cond) {
    assert(condition != kInvalidID);
    reads.emplace(condition);
}

ID BranchIfTrueHIR::proposeValue(ID /* proposedId */) {
    return kInvalidID;
}

bool BranchIfTrueHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        assert(original == condition);
        condition = replacement;
        return true;
    }

    return false;
}

void BranchIfTrueHIR::lower(LinearFrame* linearFrame) const {
    linearFrame->append(kInvalidID, std::make_unique<lir::BranchIfTrueLIR>(linearFrame->hirToReg(condition), blockId));
}

} // namespace hir
} // namespace hadron
