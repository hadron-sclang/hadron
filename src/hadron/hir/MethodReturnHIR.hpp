#ifndef SRC_HADRON_HIR_METHOD_RETURN_HIR_HPP_
#define SRC_HADRON_HIR_METHOD_RETURN_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct MethodReturnHIR : public HIR {
    MethodReturnHIR() = delete;
    explicit MethodReturnHIR(NVID retVal);
    virtual ~MethodReturnHIR() = default;

    NVID returnValue;

    // Always returns an invalid value, as this is a read-only operation.
    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<lir::LIR*>& vRegs,
            std::list<std::unique_ptr<lir::LIR>>& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_METHOD_RETURN_HIR_HPP_