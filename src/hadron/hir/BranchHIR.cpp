#include "hadron/hir/BranchHIR.hpp"

#include "hadron/lir/BranchLIR.hpp"

namespace hadron {
namespace hir {

BranchHIR::BranchHIR(): HIR(kBranch) {}

NVID BranchHIR::proposeValue(NVID /* id */) {
    return kInvalidNVID;
}

bool BranchHIR::replaceInput(NVID /* original */, NVID /* replacement */) {
    return false;
}

void BranchHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& append) const {
    append.emplace_back(std::make_unique<lir::BranchLIR>(blockId));
}

} // namespace hir
} // namespace hadron
