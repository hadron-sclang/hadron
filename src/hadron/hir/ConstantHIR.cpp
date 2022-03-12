#include "hadron/hir/ConstantHIR.hpp"

#include "hadron/lir/LoadConstantLIR.hpp"

namespace hadron {
namespace hir {

ConstantHIR::ConstantHIR(const Slot c): HIR(kConstant, c.getType(), library::Symbol()), constant(c) {}

ConstantHIR::ConstantHIR(const Slot c, library::Symbol name):
        HIR(kConstant, c.getType(), name), constant(c) {}

NVID ConstantHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool ConstantHIR::replaceInput(NVID /* original */, NVID /* replacement */) {
    return false;
}

void ConstantHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    append.emplace_back(std::make_unique<lir::LoadConstantLIR>(vReg(), constant));
    vRegs[vReg()] = --(append.end());
}

} // namespace hir
} // namespace hadron
