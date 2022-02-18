#include "hadron/hir/MessageHIR.hpp"

#include "hadron/lir/LoadConstantLIR.hpp"

namespace hadron {
namespace hir {

MessageHIR::MessageHIR(): HIR(kMessage, Type::kAny, library::Symbol()) {}

NVID MessageHIR::proposeValue(NVID id) {
    value.id = id;
    return id;
}

void MessageHIR::lower(const std::vector<HIR*>& /* values */, std::vector<LIRList::iterator>& vRegs,
        LIRList& append) const {
    // TODO: implement me
    append.emplace_back(std::make_unique<lir::LoadConstantLIR>(vReg(), Slot::makeNil()));
    vRegs[vReg()] = --(append.end());
}

} // namespace hir
} // namespace hadron
