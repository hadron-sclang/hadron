#ifndef SRC_HADRON_HIR_IMPORT_NAME_HIR_HPP_
#define SRC_HADRON_HIR_IMPORT_NAME_HIR_HPP_

#include "hadron/hir/HIR.hpp"

namespace hadron {
namespace hir {

struct ImportNameHIR : public HIR {
    enum Kind {
        kClassVariable,
        kInstanceVariable,
        kCapturedLocal
    };

    ImportNameHIR() = delete;

    // For captured local variables.
    ImportNameHIR(library::Symbol name, TypeFlags t, NVID extId);

    // For class and instance variables.
    ImportNameHIR(library::Symbol name, Kind k, int32_t off);

    virtual ~ImportNameHIR() = default;

    Kind kind;
    int32_t offset;
    NVID externalNVID;

    NVID proposeValue(NVID id) override;
    void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs, LIRList& append) const override;
};

} // namespace hir
} // namespace hadron

#endif