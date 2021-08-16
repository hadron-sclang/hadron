#include "hadron/HIR.hpp"

namespace hadron {

namespace hir {

///////////////////////////////
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

///////////////////////////////
// LoadArgumentTypeHIR
bool LoadArgumentTypeHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kLoadArgumentType) {
        return false;
    }
    const auto loadArgType = reinterpret_cast<const LoadArgumentTypeHIR*>(hir);
    return (frame == loadArgType->frame) && (index == loadArgType->index);
}

Value LoadArgumentTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

///////////////////////////////
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

///////////////////////////////
// StoreReturnHIR
StoreReturnHIR::StoreReturnHIR(Frame* f, std::pair<Value, Value> retVal):
    HIR(kStoreReturn), frame(f), returnValue(retVal) {
    reads.emplace(retVal.first);
    reads.emplace(retVal.second);
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

///////////////////////////////
// ResolveTypeHIR
bool ResolveTypeHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kResolveType) {
        return false;
    }
    const auto resolveTypeHIR = reinterpret_cast<const ResolveTypeHIR*>(hir);
    return (typeOfValue == resolveTypeHIR->typeOfValue);
}

Value ResolveTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

///////////////////////////////
// PhiHIR
void PhiHIR::addInput(Value v) {
    inputs.emplace_back(v);
    reads.emplace(v);
}

Value PhiHIR::getTrivialValue() const {
    // More than two distinct values means that even if one of them is self-referential this phi has two or more
    // non self-referential distinct values and is therefore nontrivial.
    if (reads.size() > 2) {
        return Value();
    }

    // Exactly two distinct values means that if either of the two values are self-referential than the phi is trivial
    // and we should return the other non-self-referential value.
    if (reads.size() == 2) {
        Value nonSelf;
        bool trivial = false;
        for (auto v : reads) {
            if (v != value) {
                nonSelf = v;
            } else {
                trivial = true;
            }
        }
        if (trivial) {
            return nonSelf;
        }
    }

    assert(reads.size());
    return *(reads.begin());
}

Value PhiHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

bool PhiHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kPhi) {
        return false;
    }
    const PhiHIR* phi = reinterpret_cast<const PhiHIR*>(hir);
    // Empty phis are not equivalent to any other phi.
    if (inputs.size() == 0 || phi->inputs.size() == 0) {
        return false;
    }
    if (inputs.size() != phi->inputs.size()) {
        return false;
    }
    // Order has to be the same, too.
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (inputs[i] != phi->inputs[i]) {
            return false;
        }
    }
    return true;
}

///////////////////////////////
// IfHIR
IfHIR::IfHIR(std::pair<Value, Value> cond): HIR(kIf), condition(cond) {
    reads.emplace(cond.first);
    reads.emplace(cond.second);
}

Value IfHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

bool IfHIR::isEquivalent(const HIR* /* hir */) const {
    return false;
}

///////////////////////////////
// LabelHIR
bool LabelHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kLabel) {
        return false;
    }
    const auto label = reinterpret_cast<const LabelHIR*>(hir);
    return blockNumber == label->blockNumber;
}

Value LabelHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

///////////////////////////////
// Dispatch
bool Dispatch::isEquivalent(const HIR* /* hir */) const {
    return false;
}

///////////////////////////////
// DispatchCallHIR
void DispatchCallHIR::addKeywordArgument(std::pair<Value, Value> key, std::pair<Value, Value> keyValue) {
    reads.insert(key.first);
    keywordArguments.emplace_back(key.first);
    reads.insert(key.second);
    keywordArguments.emplace_back(key.second);
    reads.insert(keyValue.first);
    keywordArguments.emplace_back(keyValue.first);
    reads.insert(keyValue.second);
    keywordArguments.emplace_back(keyValue.second);
}

void DispatchCallHIR::addArgument(std::pair<Value, Value> argument) {
    reads.insert(argument.first);
    arguments.emplace_back(argument.first);
    reads.insert(argument.second);
    arguments.emplace_back(argument.second);
}

Value DispatchCallHIR::proposeValue(uint32_t number) {
    value.number = number;
    assert(arguments.size());
    value.typeFlags = arguments[0].typeFlags;
    return value;
}

///////////////////////////////
// DispatchLoadReturnHIR
Value DispatchLoadReturnHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

///////////////////////////////
// DispatchLoadReturnTypeHIR
Value DispatchLoadReturnTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

///////////////////////////////
// DispatchCleanupHIR
Value DispatchCleanupHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}


} // namespace hir

} // namespace hadron