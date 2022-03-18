#ifndef SRC_HADRON_HIR_IMPORT_LOCAL_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_IMPORT_LOCAL_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ImportLocalVariableHIR : public HIR {
    ImportLocalVariableHIR() = delete;
    ImportLocalVariableHIR(TypeFlags t, ID extId);
    virtual ~ImportLocalVariableHIR() = default;

    ID externalId;

    ID proposeValue(ID proposedId) override;
    bool replaceInput(ID original, ID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_IMPORT_LOCAL_VARIABLE_HIR_HPP_