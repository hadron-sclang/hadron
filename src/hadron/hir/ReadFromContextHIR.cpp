#include "hadron/hir/ReadFromContextHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LoadFromPointerLIR.hpp"

namespace hadron {
namespace hir {

ReadFromContextHIR::ReadFromContextHIR(int32_t off, library::Symbol name):
        HIR(kReadFromContext, TypeFlags::kAllFlags), offset(off), valueName(name) {}

ID ReadFromContextHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ReadFromContextHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void ReadFromContextHIR::lower(LinearFrame* linearFrame) const {
    linearFrame->append(id, std::make_unique<lir::LoadFromPointerLIR>(lir::kContextPointerVReg, offset));
}

} // namespace hir
} // namespace hadron