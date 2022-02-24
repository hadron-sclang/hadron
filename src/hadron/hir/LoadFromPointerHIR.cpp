#include "hadron/hir/LoadFromPointerHIR.hpp"

#include "hadron/lir/LoadImmediateLIR.hpp"

namespace hadron {
namespace hir {

LoadFromPointerHIR::LoadFromPointerHIR(const void* p):
    HIR(kLoadFromPointer, TypeFlags::kAllFlags, library::Symbol()),
    pointer(p) {}

NVID LoadFromPointerHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void LoadFromPointerHIR::lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    append.emplace_back(std::make_unique<lir::LoadImmediateLIR>(vReg(), pointer));
    vRegs[vReg()] = --(append.end());
}

} // namespace hir
} // namespace hadron