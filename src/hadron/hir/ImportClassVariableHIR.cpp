#include "hadron/hir/ImportClassVariableHIR.hpp"

namespace hadron {
namespace hir {

ImportClassVariableHIR::ImportClassVariableHIR(library::Symbol name, library::Class def, int32_t off):
        HIR(kImportClassVariable, TypeFlags::kAllFlags, name),
        classDef(def),
        offset(off) {}

NVID ImportClassVariableHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool ImportClassVariableHIR::replaceInput(NVID /* original */, NVID /* replacement */) {
    return false;
}

void ImportClassVariableHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron