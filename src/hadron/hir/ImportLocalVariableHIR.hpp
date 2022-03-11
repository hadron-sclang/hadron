#ifndef SRC_HADRON_HIR_IMPORT_LOCAL_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_IMPORT_LOCAL_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ImportLocalVariableHIR : public HIR {
    ImportLocalVariableHIR() = delete;
    ImportLocalVariableHIR(library::Symbol name, TypeFlags t, NVID extId);
    virtual ~ImportLocalVariableHIR() = default;

    NVID externalId;

    NVID proposeValue(NVID id) override;
    bool replaceInput(NVID original, NVID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_IMPORT_LOCAL_VARIABLE_HIR_HPP_