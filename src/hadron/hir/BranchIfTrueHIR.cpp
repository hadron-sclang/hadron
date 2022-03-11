#include "hadron/hir/BranchIfTrueHIR.hpp"

#include "hadron/lir/BranchIfTrueLIR.hpp"

namespace hadron {
namespace hir {

BranchIfTrueHIR::BranchIfTrueHIR(NVID cond): HIR(kBranchIfTrue), condition(cond) {
    assert(condition != kInvalidNVID);
    reads.emplace(condition);
}

NVID BranchIfTrueHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

bool BranchIfTrueHIR::replaceInput(NVID original, NVID replacement) {
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
