#ifndef SRC_HADRON_HIR_METHOD_RETURN_HIR_HPP_
#define SRC_HADRON_HIR_METHOD_RETURN_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct MethodReturnHIR : public HIR {
    MethodReturnHIR();
    virtual ~MethodReturnHIR() = default;

    // Always returns an invalid value, as this is a read-only operation.
    NVID proposeValue(NVID id) override;
    bool replaceInput(NVID original, NVID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_METHOD_RETURN_HIR_HPP_