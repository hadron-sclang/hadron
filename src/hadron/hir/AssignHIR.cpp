#include "hadron/hir/AssignHIR.hpp"

#include "hadron/lir/AssignLIR.hpp"

namespace hadron {
namespace hir {

AssignHIR::AssignHIR(library::Symbol n, HIR* v):
    HIR(kAssign, v->value.typeFlags, n), assignValue(v->value.id) { reads.emplace(assignValue); }

NVID AssignHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool AssignHIR::replaceInput(NVID original, NVID replacement) {
    if (replaceReads(original, replacement)) {
        assert(assignValue == original);
        assignValue = replacement;
        return true;
    }

    return false;
}

void AssignHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const {
    append.emplace_back(std::make_unique<lir::AssignLIR>(vReg(), values[assignValue]->vReg(),
            values[assignValue]->value.typeFlags));
    vRegs[vReg()] = --(append.end());
}

} // namespace hir
} // namespace hadron