#include "hadron/hir/ReadFromFrameHIR.hpp"

namespace hadron {
namespace hir {

ReadFromFrameHIR::ReadFromFrameHIR(int index, library::Symbol name):
    HIR(kReadFromFrame, TypeFlags::kAllFlags), frameIndex(index), valueName(name) {}

ID ReadFromFrameHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ReadFromFrameHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void ReadFromFrameHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& /* vRegs */,
        LIRList& /* append */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron