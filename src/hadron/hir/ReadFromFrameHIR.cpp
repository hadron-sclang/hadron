#include "hadron/hir/ReadFromFrameHIR.hpp"

namespace hadron {
namespace hir {

ReadFromFrameHIR::ReadFromFrameHIR(int32_t index, hir::ID framePointer, library::Symbol name):
        HIR(kReadFromFrame, TypeFlags::kAllFlags), frameIndex(index), frameId(framePointer), valueName(name) {
    if (frameId != hir::kInvalidID) {
        reads.emplace(frameId);
    }
}

ID ReadFromFrameHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ReadFromFrameHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        assert(original == frameId);
        frameId = replacement;
        return true;
    }

    return false;
}

void ReadFromFrameHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* /* linearFrame */) const {
    assert(false);
}

} // namespace hir
} // namespace hadron