#ifndef SRC_HADRON_HIR_IMPORT_INSTANCE_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_IMPORT_INSTANCE_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ImportInstanceVariableHIR : public HIR {
    ImportInstanceVariableHIR() = delete;
    ImportInstanceVariableHIR(library::Symbol name, NVID instance, int32_t off);
    virtual ~ImportInstanceVariableHIR() = default;

    NVID thisId;
    int32_t offset;

    NVID proposeValue(NVID id) override;
    bool replaceInput(NVID original, NVID replacement) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_IMPORT_INSTANCE_VARIABLE_HIR_HPP_