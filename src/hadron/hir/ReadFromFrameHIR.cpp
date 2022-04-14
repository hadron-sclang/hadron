#include "hadron/hir/ReadFromFrameHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LoadFromFrameLIR.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"

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

void ReadFromFrameHIR::lower(LinearFrame* linearFrame) const {
    if (frameId != hir::kInvalidID) {
        auto frameVReg = linearFrame->hirToReg(frameId);
        linearFrame->append(id, std::make_unique<lir::LoadFromPointerLIR>(frameVReg, frameIndex));
        return;
    }

    linearFrame->append(id, std::make_unique<lir::LoadFromFrameLIR>(frameIndex));
}

} // namespace hir
} // namespace hadron