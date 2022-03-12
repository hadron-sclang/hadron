#include "hadron/hir/AssignHIR.hpp"

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

void AssignHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    assert(false); // WRITEME
}

} // namespace hir
} // namespace hadron