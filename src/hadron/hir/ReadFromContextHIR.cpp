#include "hadron/hir/ReadFromContextHIR.hpp"

namespace hadron {
namespace hir {

ReadFromContextHIR::ReadFromContextHIR(int32_t off, library::Symbol name):
        HIR(kReadFromContext, TypeFlags::kAllFlags), offset(off), valueName(name) {}

ID ReadFromContextHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ReadFromContextHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void ReadFromContextHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron