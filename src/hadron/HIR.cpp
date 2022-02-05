#include "hadron/HIR.hpp"

namespace hadron {

namespace hir {

///////////////////////////////
// HIR
int HIR::numberOfReservedRegisters() const {
    return 0;
}

///////////////////////////////
// LoadArgumentHIR
Value LoadArgumentHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = isVarArgs ? Type::kArray : Type::kAny;
    return value;
}

///////////////////////////////
// LoadArgumentTypeHIR
Value LoadArgumentTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

///////////////////////////////
// ConstantHIR
Value ConstantHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = constant.getType();
    return value;
}

///////////////////////////////
// StoreReturnHIR
StoreReturnHIR::StoreReturnHIR(std::pair<Value, Value> retVal):
    HIR(kStoreReturn), returnValue(retVal) {
    reads.emplace(retVal.first);
    reads.emplace(retVal.second);
}

Value StoreReturnHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

///////////////////////////////
// LoadInstanceVariableHIR
LoadInstanceVariableHIR::LoadInstanceVariableHIR(std::pair<Value, Value> thisVal, int32_t index):
        HIR(kLoadInstanceVariable),
        thisValue(thisVal),
        variableIndex(index) {
    reads.emplace(thisVal.first);
    reads.emplace(thisVal.second);
}

Value LoadInstanceVariableHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

///////////////////////////////
// LoadInstanceVariableTypeHIR
LoadInstanceVariableTypeHIR::LoadInstanceVariableTypeHIR(std::pair<Value, Value> thisVal, int32_t index):
        HIR(kLoadInstanceVariableType),
        thisValue(thisVal),
        variableIndex(index) {
    reads.emplace(thisVal.first);
    reads.emplace(thisVal.second);
}

Value LoadInstanceVariableTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

///////////////////////////////
// LoadClassVariableHIR
LoadClassVariableHIR::LoadClassVariableHIR(std::pair<Value, Value> thisVal, int32_t index):
        HIR(kLoadInstanceVariable),
        thisValue(thisVal),
        variableIndex(index) {
    reads.emplace(thisVal.first);
    reads.emplace(thisVal.second);
}

Value LoadClassVariableHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kAny;
    return value;
}

///////////////////////////////
// LoadClassVariableTypeHIR
LoadClassVariableTypeHIR::LoadClassVariableTypeHIR(std::pair<Value, Value> thisVal, int32_t index):
        HIR(kLoadInstanceVariableType),
        thisValue(thisVal),
        variableIndex(index) {
    reads.emplace(thisVal.first);
    reads.emplace(thisVal.second);
}

Value LoadClassVariableTypeHIR::proposeValue(uint32_t number) {
    value.number = number;
    value.typeFlags = Type::kType;
    return value;
}

///////////////////////////////
// StoreInstanceVariableHIR
StoreInstanceVariableHIR::StoreInstanceVariableHIR(std::pair<Value, Value> thisVal, std::pair<Value, Value> storeVal,
        int32_t index):
        HIR(kStoreInstanceVariable),
        thisValue(thisVal),
        storeValue(storeVal),
        variableIndex(index) {
    reads.emplace(thisVal.first);
    reads.emplace(thisVal.second);
    reads.emplace(storeVal.first);
    reads.emplace(storeVal.second);
}

Value StoreInstanceVariableHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

///////////////////////////////
// StoreClassVariableHIR
StoreClassVariableHIR::StoreClassVariableHIR(std::pair<Value, Value> thisVal, std::pair<Value, Value> storeVal,
        int32_t index):
        HIR(kStoreInstanceVariable),
        thisValue(thisVal),
        storeValue(storeVal),
        variableIndex(index) {
    reads.emplace(thisVal.first);
    reads.emplace(thisVal.second);
    reads.emplace(storeVal.first);
    reads.emplace(storeVal.second);
}

Value StoreClassVariableHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
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

///////////////////////////////
// BranchHIR
Value BranchHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

///////////////////////////////
// BranchIfTrueHIR
BranchIfTrueHIR::BranchIfTrueHIR(std::pair<Value, Value> cond): HIR(kBranchIfTrue), condition(cond) {
    reads.emplace(cond.first);
    reads.emplace(cond.second);
}

Value BranchIfTrueHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

///////////////////////////////
// LabelHIR
Value LabelHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

///////////////////////////////
// DispatchSetupStackHIR
DispatchSetupStackHIR::DispatchSetupStackHIR(std::pair<Value, Value> selector, int numArgs, int numKeyArgs):
        HIR(kDispatchSetupStack),
        selectorValue(selector),
        numberOfArguments(numArgs),
        numberOfKeywordArguments(numKeyArgs) {
    reads.emplace(selectorValue.first);
    reads.emplace(selectorValue.second);
}

Value DispatchSetupStackHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

int DispatchSetupStackHIR::numberOfReservedRegisters() const {
    // We save one for the frame pointer in all the Dispatch Stack Setup commands. They must be issued contiguously.
    return 1;
}

///////////////////////////////
// DispatchStoreArgHIR
DispatchStoreArgHIR::DispatchStoreArgHIR(int argNum, std::pair<Value, Value> argVal):
        HIR(kDispatchStoreArg),
        argumentNumber(argNum),
        argumentValue(argVal) {
    reads.emplace(argumentValue.first);
    reads.emplace(argumentValue.second);
}

Value DispatchStoreArgHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

int DispatchStoreArgHIR::numberOfReservedRegisters() const {
    // Still saving one for the frame pointer
    return 1;
}

///////////////////////////////
// DispatchStoreKeyArgHIR
DispatchStoreKeyArgHIR::DispatchStoreKeyArgHIR(int keyArgNum, std::pair<Value, Value> key,
        std::pair<Value, Value> keyVal):
        HIR(kDispatchStoreKeyArg),
        keywordArgumentNumber(keyArgNum),
        keyword(key),
        keywordValue(keyVal) {
    reads.emplace(keyword.first);
    reads.emplace(keyword.second);
    reads.emplace(keyVal.first);
    reads.emplace(keyVal.second);
}

Value DispatchStoreKeyArgHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

int DispatchStoreKeyArgHIR::numberOfReservedRegisters() const {
    // Still hopefully saving one for the frame pointer
    return 1;
}

///////////////////////////////
// DispatchCallHIR
Value DispatchCallHIR::proposeValue(uint32_t /* number */) {
    value.number = 0;
    value.typeFlags = 0;
    return value;
}

int DispatchCallHIR::numberOfReservedRegisters() const {
    // Mark every register as reserved, to force the register allocator to save every active value to the stack.
    return -1;
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