#ifndef SRC_HADRON_HIR_READ_FROM_CONTEXT_HIR_HPP_
#define SRC_HADRON_HIR_READ_FROM_CONTEXT_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ReadFromContextHIR : public HIR {
    ReadFromContextHIR() = delete;
    ReadFromContextHIR(int32_t off, library::Symbol name);
    virtual ~ReadFromContextHIR() = default;

    int32_t offset;
    library::Symbol valueName;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_READ_FROM_CONTEXT_HIR_HPP_