#include "hadron/HIR.hpp"

namespace hadron {

namespace hir {

bool LoadArgumentHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kLoadArgument) {
        return false;
    }
    const auto loadArg = reinterpret_cast<const LoadArgumentHIR*>(hir);
    return (index == loadArg->index) && (loadValue == loadArg->loadValue);
}

bool ConstantHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kConstant) {
        return false;
    }
    const auto constant = reinterpret_cast<const ConstantHIR*>(hir);
    return value == constant->value;
}

} // namespace hir

} // namespace hadron