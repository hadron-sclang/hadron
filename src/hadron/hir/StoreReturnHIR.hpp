#ifndef SRC_HADRON_HIR_STORE_RETURN_HIR_HPP_
#define SRC_HADRON_HIR_STORE_RETURN_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct StoreReturnHIR : public HIR {
    StoreReturnHIR() = delete;
    explicit StoreReturnHIR(ID value);
    virtual ~StoreReturnHIR() = default;

    ID returnValue;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(LinearFrame* linearFrame) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_STORE_RETURN_HIR_HPP_