#include "hadron/HIR.hpp"

namespace hadron {

namespace hir {

//////////////////////////
// LoadArgumentHIR
bool LoadArgumentHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kLoadArgument) {
        return false;
    }
    const auto loadArg = reinterpret_cast<const LoadArgumentHIR*>(hir);
    return (frame == loadArg->frame) && (index == loadArg->index);
}

Value LoadArgumentHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

//////////////////////////
// ConstantHIR
bool ConstantHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kConstant) {
        return false;
    }
    const auto constantHIR = reinterpret_cast<const ConstantHIR*>(hir);
    return (constant == constantHIR->constant);
}

Value ConstantHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = constant.type;
    return value;
}

//////////////////////////
// StoreReturnHIR
StoreReturnHIR::StoreReturnHIR(Frame* f, Value retVal): HIR(kStoreReturn), frame(f), returnValue(retVal) {
    reads.emplace(retVal);
}

bool StoreReturnHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kStoreReturn) {
        return false;
    }
    const auto storeReturn = reinterpret_cast<const StoreReturnHIR*>(hir);
    return (frame == storeReturn->frame) && (returnValue == storeReturn->returnValue);
}

Value StoreReturnHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

//////////////////////////
// IfHIR
IfHIR::IfHIR(Value cond): HIR(kIf), condition(cond) {
    reads.emplace(cond);
}

Value IfHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

bool IfHIR::isEquivalent(const HIR* /* hir */) const {
    return false;
}

//////////////////////////
// Dispatch
bool Dispatch::isEquivalent(const HIR* /* hir */) const {
    return false;
}

//////////////////////////
// DispatchCallHIR
void DispatchCallHIR::addKeywordArgument(Value key, Value keyValue) {
    reads.insert(key);
    keywordArguments.emplace_back(key);
    reads.insert(keyValue);
    keywordArguments.emplace_back(keyValue);
}

void DispatchCallHIR::addArgument(Value argument) {
    reads.insert(argument);
    arguments.emplace_back(argument);
}

Value DispatchCallHIR::proposeValue(uint32_t number) {
    value.number = number;
    assert(arguments.size());
    value.typeFlags = arguments[0].typeFlags;
    return value;
}

//////////////////////////
// DispatchLoadReturnHIR
Value DispatchLoadReturnHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

//////////////////////////
// DispatchCleanupHIR
Value DispatchCleanupHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}


} // namespace hir

} // namespace hadron