#ifndef SRC_HADRON_HIR_CONSTANT_HIR_HPP_
#define SRC_HADRON_HIR_CONSTANT_HIR_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/Slot.hpp"

namespace hadron {
namespace hir {

struct ConstantHIR : public HIR {
    ConstantHIR() = delete;
    explicit ConstantHIR(const Slot c); // makes an anonymous constant
    ConstantHIR(const Slot c, library::Symbol name);
    virtual ~ConstantHIR() = default;
    Slot constant;

    // Forces the type of the |constant| Slot.
    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_CONSTANT_HIR_HPP_