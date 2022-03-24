#ifndef SRC_HADRON_HIR_READ_FROM_CLASS_HIR_HPP_
#define SRC_HADRON_HIR_READ_FROM_CLASS_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ReadFromClassHIR : public HIR {
    ReadFromClassHIR() = delete;
    ReadFromClassHIR(hir::ID classArray, int index, library::Symbol name);
    virtual ~ReadFromClassHIR() = default;

    hir::ID classVariableArray;
    int arrayIndex;
    library::Symbol valueName;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_READ_FROM_CLASS_HIR_HPP_