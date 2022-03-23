#include "hadron/hir/ReadFromFrameHIR.hpp"

namespace hadron {
namespace hir {

ReadFromFrameHIR::ReadFromFrameHIR(int index, library::Symbol name):
    frameIndex(index), valueName(name) {}

ID LoadArgumentHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool LoadArgumentHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void LoadArgumentHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    assert(false);
}

} // namespace hir
} // namespace hadron