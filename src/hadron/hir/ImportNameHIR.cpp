#include "hadron/hir/ImportNameHIR.hpp"

namespace hadron {
namespace hir {

ImportNameHIR::ImportNameHIR(library::Symbol name, TypeFlags t, NVID extId):
        HIR(kImportName, t, name),
        kind(Kind::kCapturedLocal),
        offset(-1),
        externalNVID(extId) {}

ImportNameHIR::ImportNameHIR(library::Symbol name, Kind k, int32_t off):
        HIR(kImportName, TypeFlags::kAllFlags, name),
        kind(k),
        offset(off),
        externalNVID(hir::kInvalidNVID) {}


NVID ImportNameHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void ImportNameHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    // WRITEME
    assert(false);
}

} // namespace hir
} // namespace hadron