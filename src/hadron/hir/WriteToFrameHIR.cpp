#include "hadron/hir/WriteToFrameHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/StoreToFrameLIR.hpp"
#include "hadron/lir/StoreToPointerLIR.hpp"

namespace hadron {
namespace hir {

WriteToFrameHIR::WriteToFrameHIR(int32_t index, hir::ID framePointer, library::Symbol name, hir::ID v):
        HIR(kWriteToFrame),
        frameIndex(index),
        frameId(framePointer),
        valueName(name),
        toWrite(v) {

    reads.emplace(toWrite);

    if (frameId != hir::kInvalidID) {
        reads.emplace(frameId);
    }
}

ID WriteToFrameHIR::proposeValue(ID /* proposedId */) {
    return kInvalidID;
}

bool WriteToFrameHIR::replaceInput(ID original, ID replacement) {
    if (replaceReads(original, replacement)) {
        if (frameId == original) {
            frameId = replacement;
        }
        if (toWrite == original) {
            toWrite = replacement;
        }
        return true;
    }

    return false;
}

void WriteToFrameHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* linearFrame) const {
    auto toWriteVReg = linearFrame->hirToReg(toWrite);
    if (frameId == kInvalidID) {
        linearFrame->append(kInvalidID, std::make_unique<lir::StoreToFrameLIR>(toWriteVReg, frameIndex));
        return;
    }

    auto frameIdVReg = linearFrame->hirToReg(frameId);
    linearFrame->append(kInvalidID, std::make_unique<lir::StoreToPointerLIR>(frameIdVReg, toWriteVReg, frameIndex));
}

} // namespace hir
} // namespace hadron
