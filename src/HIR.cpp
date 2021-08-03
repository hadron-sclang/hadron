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



} // namespace hir

} // namespace hadron