#ifndef SRC_HADRON_HIR_IMPORT_CLASS_VARIABLE_HIR_HPP_
#define SRC_HADRON_HIR_IMPORT_CLASS_VARIABLE_HIR_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/library/Kernel.hpp"

namespace hadron {
namespace hir {

struct ImportClassVariableHIR : public HIR {
    ImportClassVariableHIR() = delete;
    // Offset is the offset within the class variables of the class only.
    ImportClassVariableHIR(library::Symbol name, library::Class def, int32_t off);
    virtual ~ImportClassVariableHIR() = default;

    library::Class classDef;
    int32_t offset;

    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_IMPORT_CLASS_VARIABLE_HIR_HPP_