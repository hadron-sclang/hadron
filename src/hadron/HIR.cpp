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
    return index == loadArg->index;
}

Value LoadArgumentHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

int LoadArgumentHIR::numberOfReservedRegisters() const {
    return 0;
}

///////////////////////////////
// LoadArgumentTypeHIR
bool LoadArgumentTypeHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kLoadArgumentType) {
        return false;
    }
    const auto loadArgType = reinterpret_cast<const LoadArgumentTypeHIR*>(hir);
    return index == loadArgType->index;
}

Value LoadArgumentTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

int LoadArgumentTypeHIR::numberOfReservedRegisters() const {
    return 0;
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
    value.typeFlags = constant.getType();
    return value;
}

int ConstantHIR::numberOfReservedRegisters() const {
    return 0;
}

///////////////////////////////
// StoreReturnHIR
StoreReturnHIR::StoreReturnHIR(std::pair<Value, Value> retVal):
    HIR(kStoreReturn), returnValue(retVal) {
    reads.emplace(retVal.first);
    reads.emplace(retVal.second);
}

bool StoreReturnHIR::isEquivalent(const HIR* hir) const {
    if (hir->opcode != kStoreReturn) {
        return false;
    }
    const auto storeReturn = reinterpret_cast<const StoreReturnHIR*>(hir);
    return returnValue == storeReturn->returnValue;
}

Value StoreReturnHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

int StoreReturnHIR::numberOfReservedRegisters() const {
    return 0;
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

int ResolveTypeHIR::numberOfReservedRegisters() const {
    return 0;
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

int PhiHIR::numberOfReservedRegisters() const {
    return 0;
}

///////////////////////////////
// BranchHIR
Value BranchHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

bool BranchHIR::isEquivalent(const HIR* /* hir */) const {
    return false;
}

int BranchHIR::numberOfReservedRegisters() const {
    return 0;
}

///////////////////////////////
// BranchIfZeroHIR
BranchIfZeroHIR::BranchIfZeroHIR(std::pair<Value, Value> cond): HIR(kBranchIfZero), condition(cond) {
    reads.emplace(cond.first);
    reads.emplace(cond.second);
}

Value BranchIfZeroHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

bool BranchIfZeroHIR::isEquivalent(const HIR* /* hir */) const {
    return false;
}

int BranchIfZeroHIR::numberOfReservedRegisters() const {
    return 0;
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

int LabelHIR::numberOfReservedRegisters() const {
    return 0;
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

int DispatchCallHIR::numberOfReservedRegisters() const {
    // Launch every ZERG! For great Justice!
    return -1;
}

///////////////////////////////
// DispatchLoadReturnHIR
Value DispatchLoadReturnHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

int DispatchLoadReturnHIR::numberOfReservedRegisters() const {
    return 0;
}

///////////////////////////////
// DispatchLoadReturnTypeHIR
Value DispatchLoadReturnTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

int DispatchLoadReturnTypeHIR::numberOfReservedRegisters() const {
    return 0;
}

///////////////////////////////
// DispatchCleanupHIR
Value DispatchCleanupHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

int DispatchCleanupHIR::numberOfReservedRegisters() const {
    return 0;
}

} // namespace hir

} // namespace hadron