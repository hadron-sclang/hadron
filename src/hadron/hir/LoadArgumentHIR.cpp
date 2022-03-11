#include "hadron/hir/LoadArgumentHIR.hpp"

#include "hadron/lir/LoadFromStackLIR.hpp"

namespace hadron {
namespace hir {

LoadArgumentHIR::LoadArgumentHIR(int index, library::Symbol name):
        HIR(kLoadArgument, TypeFlags::kAllFlags, name),
        argIndex(index) { }

NVID LoadArgumentHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

bool LoadArgumentHIR::replaceInput(NVID /* original */, NVID /* replacement */) {
    return false;
}

void LoadArgumentHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    // Arguments start at sp - 1 for argument 0, so subtract 1 from index for the stack pointer offset.
    append.emplace_back(std::make_unique<lir::LoadFromStackLIR>(vReg(), false, argIndex - 1));
    vRegs[vReg()] = --(append.end());
}

} // namespace hir
} // namespace hadron