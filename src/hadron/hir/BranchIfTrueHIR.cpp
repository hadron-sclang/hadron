#include "hadron/hir/BranchIfTrueHIR.hpp"

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

void BranchIfTrueHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& append) const {
    append.emplace_back(std::make_unique<lir::BranchIfTrueLIR>(values[condition]->vReg(), blockId));
}

} // namespace hir
} // namespace hadron
