#include "hadron/hir/ImportInstanceVariableHIR.hpp"

namespace hadron {
namespace hir {

ImportInstanceVariableHIR::ImportInstanceVariableHIR(library::Symbol name, NVID instance, int32_t off):
        HIR(kImportInstanceVariable, TypeFlags::kAllFlags, name),
        thisId(instance),
        offset(off) {
    reads.emplace(instance);
}

NVID ImportInstanceVariableHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool ImportInstanceVariableHIR::replaceInput(NVID original, NVID replacement) {
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