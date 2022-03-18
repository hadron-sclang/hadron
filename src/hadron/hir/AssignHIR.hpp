#ifndef SRC_HADRON_HIR_ASSIGN_HIR_HPP_
#define SRC_HADRON_HIR_ASSIGN_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct AssignHIR : public HIR {
    AssignHIR() = delete;
    AssignHIR(library::Symbol n, ID value);
    virtual ~AssignHIR() = default;

    library::Symbol name;
    ID valueId;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_ASSIGN_HIR_HPP_