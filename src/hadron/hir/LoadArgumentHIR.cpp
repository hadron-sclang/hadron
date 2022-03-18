#include "hadron/hir/LoadArgumentHIR.hpp"

#include "hadron/lir/LoadFromStackLIR.hpp"

namespace hadron {
namespace hir {

LoadArgumentHIR::LoadArgumentHIR(int index):
        HIR(kLoadArgument, TypeFlags::kAllFlags),
        argIndex(index) { }

ID LoadArgumentHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool LoadArgumentHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void LoadArgumentHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    // Arguments start at sp - 1 for argument 0 and are laid out in decreasing sp values from there.
    append.emplace_back(std::make_unique<lir::LoadFromStackLIR>(vReg(), false, -1 - argIndex));
    vRegs[vReg()] = --(append.end());
}

} // namespace hir
} // namespace hadron