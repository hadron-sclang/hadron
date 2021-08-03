#include "hadron/HIR.hpp"

namespace hadron {

namespace hir {

//////////////////////
// LoadArgumentHIR
bool LoadArgumentHIR::isAlwaysUnique() const {
    return false;
}

bool LoadArgumentHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kLoadArgument) {
        return false;
    }
    const auto loadArg = reinterpret_cast<const LoadArgumentHIR*>(hir);
    return (index == loadArg->index) && (loadValue == loadArg->loadValue);
}

//////////////////////
// ConstantHIR
bool ConstantHIR::isAlwaysUnique() const {
    return false;
}

bool ConstantHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kConstant) {
        return false;
    }
    const auto constant = reinterpret_cast<const ConstantHIR*>(hir);
    return value == constant->value;
}

//////////////////////
// StoreReturnHIR
StoreReturnHIR::StoreReturnHIR(std::pair<int32_t, int32_t> retVal): HIR(kStoreReturn), returnValue(retVal) {
    reads.emplace(returnValue.first);
    reads.emplace(returnValue.second);
}

bool StoreReturnHIR::isAlwaysUnique() const {
    return false;
}

bool StoreReturnHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kStoreReturn) {
        return false;
    }
    const auto storeReturn = reinterpret_cast<const StoreReturnHIR*>(hir);
    return returnValue == storeReturn->returnValue;
}

} // namespace hir

} // namespace hadron