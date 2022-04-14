#ifndef SRC_HADRON_HIR_READ_FROM_FRAME_HIR_HPP_
#define SRC_HADRON_HIR_READ_FROM_FRAME_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ReadFromFrameHIR : public HIR {
    ReadFromFrameHIR() = delete;
    // if |framePointer| is hir::kInvalidID this will use the current active frame pointer.
    ReadFromFrameHIR(int32_t index, hir::ID framePointer, library::Symbol name);
    virtual ~ReadFromFrameHIR() = default;

    int32_t frameIndex;
    hir::ID frameId;
    library::Symbol valueName;

    // Forces the kAny type for all arguments.
    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_READ_FROM_FRAME_HIR_HPP_