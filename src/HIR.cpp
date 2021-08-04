#include "hadron/HIR.hpp"

namespace hadron {

namespace hir {

//////////////////////
// LoadArgumentHIR

bool LoadArgumentHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kLoadArgument) {
        return false;
    }
    const auto loadArg = reinterpret_cast<const LoadArgumentHIR*>(hir);
    return (index == loadArg->index) && (loadValue == loadArg->loadValue);
}

//////////////////////
// ConstantHIR

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

bool StoreReturnHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kStoreReturn) {
        return false;
    }
    const auto storeReturn = reinterpret_cast<const StoreReturnHIR*>(hir);
    return returnValue == storeReturn->returnValue;
}

//////////////////////
// Dispatch
bool Dispatch::isEquivalent(const HIR* /* hir */) const {
    return false;
}

//////////////////////
// DispatchCallHIR
void DispatchCallHIR::addKeywordArgument(std::pair<int32_t, int32_t> keyword, std::pair<int32_t, int32_t> value) {
    reads.insert(keyword.first);
    reads.insert(keyword.second);
    keywordArguments.emplace_back(keyword);
    reads.insert(value.first);
    reads.insert(value.second);
    keywordArguments.emplace_back(value);
}

void DispatchCallHIR::addArgument(std::pair<int32_t, int32_t> argument) {
    reads.insert(argument.first);
    reads.insert(argument.second);
    arguments.emplace_back(argument);
}


} // namespace hir

} // namespace hadron