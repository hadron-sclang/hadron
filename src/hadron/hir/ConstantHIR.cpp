#include "hadron/hir/ConstantHIR.hpp"

#include "hadron/LinearFrame.hpp"
#include "hadron/lir/LoadConstantLIR.hpp"

namespace hadron {
namespace hir {

ConstantHIR::ConstantHIR(const Slot c): HIR(kConstant, c.getType()), constant(c) {}

ID ConstantHIR::proposeValue(ID proposedId) {
    id = proposedId;
    return id;
}

bool ConstantHIR::replaceInput(ID /* original */, ID /* replacement */) {
    return false;
}

void ConstantHIR::lower(const std::vector<HIR*>& /* values */, LinearFrame* linearFrame) const {
    linearFrame->append(id, std::make_unique<lir::LoadConstantLIR>(constant));
}

} // namespace hir
} // namespace hadron
