#ifndef SRC_HADRON_HIR_LOAD_OUTER_FRAME_HIR_HPP_
#define SRC_HADRON_HIR_LOAD_OUTER_FRAME_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct LoadOuterFrameHIR : public HIR {
    LoadOuterFrameHIR() = delete;
    // If inner is kInvalidID that means we load directly from the frame pointer, so the first level of outer contexts.
    // Higher outer contexts take the value of the next inner context as input.
    explicit LoadOuterFrameHIR(hir::ID inner);
    virtual ~LoadOuterFrameHIR() = default;

    hir::ID innerContext;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_LOAD_OUTER_FRAME_HIR_HPP_