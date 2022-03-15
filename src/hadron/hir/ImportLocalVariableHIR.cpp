#include "hadron/hir/ImportLocalVariableHIR.hpp"

namespace hadron {
namespace hir {

ImportLocalVariableHIR::ImportLocalVariableHIR(TypeFlags t, ID extId):
        HIR(kImportLocalVariable, t),
        externalId(extId) {}

ID ImportLocalVariableHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ImportLocalVariableHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void ImportLocalVariableHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron