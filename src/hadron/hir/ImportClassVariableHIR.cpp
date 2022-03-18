#include "hadron/hir/ImportClassVariableHIR.hpp"

namespace hadron {
namespace hir {

ImportClassVariableHIR::ImportClassVariableHIR(library::Class def, int32_t off):
        HIR(kImportClassVariable, TypeFlags::kAllFlags),
        classDef(def),
        offset(off) {}

ID ImportClassVariableHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ImportClassVariableHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void ImportClassVariableHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron