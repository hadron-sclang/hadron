#ifndef SRC_HADRON_HIR_STORE_RETURN_HIR_HPP_
#define SRC_HADRON_HIR_STORE_RETURN_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct StoreReturnHIR : public HIR {
    StoreReturnHIR() = delete;
    explicit StoreReturnHIR(NVID value);
    virtual ~StoreReturnHIR() = default;

    NVID returnValue;

    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_STORE_RETURN_HIR_HPP_