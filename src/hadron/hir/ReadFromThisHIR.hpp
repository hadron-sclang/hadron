#ifndef SRC_HADRON_HIR_READ_FROM_THIS_HIR_HPP_
#define SRC_HADRON_HIR_READ_FROM_THIS_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ReadFromThisHIR : public HIR {
    ReadFromThisHIR() = delete;
    ReadFromThisHIR(hir::ID tID, int32_t index, library::Symbol name);
    virtual ~ReadFromThisHIR() = default;

    hir::ID thisId;
    int32_t index;
    library::Symbol valueName;

    // Forces the kAny type for all arguments.
    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron


#endif // SRC_HADRON_HIR_READ_FROM_THIS_HIR_HPP_