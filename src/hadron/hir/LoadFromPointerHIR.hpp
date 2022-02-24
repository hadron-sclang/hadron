#ifndef SRC_HADRON_HIR_LOAD_FROM_POINTER_HPP_
#define SRC_HADRON_HIR_LOAD_FROM_POINTER_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct LoadFromPointerHIR : public HIR {
    LoadFromPointerHIR() = delete;
    explicit LoadFromPointerHIR(void* p);
    virtual ~LoadFromPointerHIR() = default;

    void* pointer;

    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_LOAD_FROM_POINTER_HPP_