#ifndef SRC_HADRON_HIR_WRITE_TO_FRAME_HIR_HPP_
#define SRC_HADRON_HIR_WRITE_TO_FRAME_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct WriteToFrameHIR : public HIR {
    WriteToFrameHIR() = delete;
    WriteToFrameHIR(int32_t index, hir::ID framePointer, library::Symbol name, hir::ID v);
    virtual ~WriteToFrameHIR() = default;

    int32_t frameIndex;
    hir::ID frameId;
    library::Symbol valueName;
    hir::ID toWrite;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_WRITE_TO_FRAME_HIR_HPP_