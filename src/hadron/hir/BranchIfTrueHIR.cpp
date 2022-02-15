#include "hadron/hir/BranchIfTrueHIR.hpp"

#include "hadron/lir/BranchIfTrueLIR.hpp"

namespace hadron {
namespace hir {

BranchIfTrueHIR::BranchIfTrueHIR(NVID cond): HIR(kBranchIfTrue), condition(cond) {}

NVID BranchIfTrueHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

void BranchIfTrueHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& append) const {
    append.emplace_back(std::make_unique<lir::BranchIfTrueLIR>(values[condition]->vReg(), blockId));
}

} // namespace hir
} // namespace hadron
