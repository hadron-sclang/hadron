#ifndef SRC_HADRON_HIR_READ_FROM_FRAME_HIR_HPP_
#define SRC_HADRON_HIR_READ_FROM_FRAME_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ReadFromFrameHIR : public HIR {
    ReadFromFrameHIR() = delete;
    ReadFromFrameHIR(int index, library::Symbol name);
    virtual ~ReadFromFrameHIR() = default;

    int frameIndex;
    library::Symbol valueName;

    // Forces the kAny type for all arguments.
    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_READ_FROM_FRAME_HIR_HPP_