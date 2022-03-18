#include "hadron/hir/ImportInstanceVariableHIR.hpp"

namespace hadron {
namespace hir {

ImportInstanceVariableHIR::ImportInstanceVariableHIR(ID instance, int32_t off):
        HIR(kImportInstanceVariable, TypeFlags::kAllFlags),
        thisId(instance),
        offset(off) {
    reads.emplace(instance);
}

ID ImportInstanceVariableHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ImportInstanceVariableHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        assert(thisId == original);
        thisId = replacement;
        return true;
    }

    return false;
}

void ImportInstanceVariableHIR::lower(const std::vector<HIR*>& /* values */,
        std::vector<LIRList::iterator>& /* vRegs */, LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron