#include "hadron/hir/ImportLocalVariableHIR.hpp"

namespace hadron {
namespace hir {

ImportLocalVariableHIR::ImportLocalVariableHIR(library::Symbol name, TypeFlags t, NVID extId):
        HIR(kImportLocalVariable, t, name),
        externalId(extId) {}

NVID ImportLocalVariableHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void ImportLocalVariableHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron