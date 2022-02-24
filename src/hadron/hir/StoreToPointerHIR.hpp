#ifndef SRC_HADRON_HIR_STORE_TO_POINTER_HIR_HPP_
#define SRC_HADRON_HIR_STORE_TO_POINTER_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct StoreToPointerHIR : public HIR {
    StoreToPointerHIR() = delete;
    StoreToPointerHIR(void* p, NVID storeVal);
    virtual ~StoreToPointerHIR() = default;

    void* pointer;
    NVID storeValue;

    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_STORE_TO_POINTER_HIR_HPP_