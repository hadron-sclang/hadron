#ifndef SRC_HADRON_HIR_HIR_HPP_
#define SRC_HADRON_HIR_HIR_HPP_

#include "hadron/Slot.hpp"
#include "hadron/library/Symbol.hpp"

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

struct Block;
struct Frame;


namespace hir {

using NVID = int32_t;
static constexpr int32_t kInvalidNVID = -1;

struct NamedValue {
    NVID id;
    int32_t typeFlags;
    library::Symbol name; // Can be nil for anonymous values
};

enum Opcode {
    kLoadArgument,
    kConstant,
    kMethodReturn,

    kLoadInstanceVariable,
    kLoadClassVariable,
    kStoreInstanceVariable,
    kStoreClassVariable,

    // Control flow
    kPhi,
    kBranch,
    kBranchIfTrue,

    kMessage
};

// All HIR instructions modify the value, thus creating a new version, and may read multiple other values, recorded in
// the reads member.
struct HIR {
    HIR() = delete;
    explicit HIR(Opcode op): opcode(op) {}
    virtual ~HIR() = default;
    Opcode opcode;
    NVID valueID;
    std::unordered_set<NVID> reads;

    // Recommended way to set the |valueID| member. Allows the HIR object to modify the proposed value type. For
    // convenience returns |value| as recorded within this object. Can return an invalid value, which indicates
    // that this operation only consumes values but doesn't generate a new one.
    virtual NamedValue proposeValue(int32_t number) = 0;
};

// Loads the argument at |index| from the stack.
struct LoadArgumentHIR : public HIR {
    LoadArgumentHIR() = delete;
    LoadArgumentHIR(int argIndex, bool varArgs = false): HIR(kLoadArgument), index(argIndex), isVarArgs(varArgs) {}
    virtual ~LoadArgumentHIR() = default;
    int index;
    bool isVarArgs;

    // Forces the kAny type for all arguments.
    Value proposeValue(uint32_t number) override;
};

// Represents the type associated with the value at |index|.
struct LoadArgumentTypeHIR : public HIR {
    LoadArgumentTypeHIR(int argIndex): HIR(kLoadArgumentType), index(argIndex) {}
    virtual ~LoadArgumentTypeHIR() = default;
    int index;

    // Forces the kType type for all arguments.
    Value proposeValue(uint32_t number) override;
};

struct ConstantHIR : public HIR {
    ConstantHIR(const Slot& c): HIR(kConstant), constant(c) {}
    virtual ~ConstantHIR() = default;
    Slot constant;

    // Forces the type of the |constant| Slot.
    Value proposeValue(uint32_t number) override;
};

struct MethodReturnHIR : public HIR {
    MethodReturnHIR() = delete;
    MethodReturnHIR(std::pair<Value, Value> retVal);
    virtual ~MethodReturnHIR() = default;
    std::pair<Value, Value> returnValue;

    // Always returns an invalid value, as this is a read-only operation.
    Value proposeValue(uint32_t number) override;
};

struct LoadInstanceVariableHIR : public HIR {
    LoadInstanceVariableHIR() = delete;
    LoadInstanceVariableHIR(std::pair<Value, Value> thisVal, int32_t index);
    virtual ~LoadInstanceVariableHIR() = default;

    // Need to load the |this| pointer to deference an instance variable.
    std::pair<Value, Value> thisValue;
    int32_t variableIndex;

    Value proposeValue(uint32_t number) override;
};

struct LoadInstanceVariableTypeHIR : public HIR {
    LoadInstanceVariableTypeHIR() = delete;
    LoadInstanceVariableTypeHIR(std::pair<Value, Value> thisVal, int32_t index);
    virtual ~LoadInstanceVariableTypeHIR() = default;

    // Need to load the |this| pointer to deference an instance variable.
    std::pair<Value, Value> thisValue;
    int32_t variableIndex;

    Value proposeValue(uint32_t number) override;
};

struct LoadClassVariableHIR : public HIR {
    LoadClassVariableHIR() = delete;
    LoadClassVariableHIR(std::pair<Value, Value> thisVal, int32_t index);
    virtual ~LoadClassVariableHIR() = default;

    // Need to load the |this| pointer to deference a class variable.
    std::pair<Value, Value> thisValue;
    int32_t variableIndex;

    Value proposeValue(uint32_t number) override;
};

struct LoadClassVariableTypeHIR : public HIR {
    LoadClassVariableTypeHIR() = delete;
    LoadClassVariableTypeHIR(std::pair<Value, Value> thisVal, int32_t index);
    virtual ~LoadClassVariableTypeHIR() = default;

    // Need to load the |this| pointer to deference class variable.
    std::pair<Value, Value> thisValue;
    int32_t variableIndex;

    Value proposeValue(uint32_t number) override;
};

struct StoreInstanceVariableHIR : public HIR {
    StoreInstanceVariableHIR() = delete;
    StoreInstanceVariableHIR(std::pair<Value, Value> thisVal, std::pair<Value, Value> storeVal, int32_t index);
    virtual ~StoreInstanceVariableHIR() = default;

    std::pair<Value, Value> thisValue;
    std::pair<Value, Value> storeValue;
    int32_t variableIndex;

    Value proposeValue(uint32_t number) override;
};

struct StoreClassVariableHIR : public HIR {
    StoreClassVariableHIR() = delete;
    StoreClassVariableHIR(std::pair<Value, Value> thisVal, std::pair<Value, Value> storeVal, int32_t index);
    virtual ~StoreClassVariableHIR() = default;

    std::pair<Value, Value> thisValue;
    std::pair<Value, Value> storeValue;
    int32_t variableIndex;

    Value proposeValue(uint32_t number) override;
};

struct PhiHIR : public HIR {
    PhiHIR(): HIR(kPhi) {}

    std::vector<Value> inputs;
    void addInput(Value v);
    // A phi is *trivial* if it has only one distinct input value that is not self-referential. If this phi is trivial,
    // return the trivial value. Otherwise return an invalid value.
    Value getTrivialValue() const;

    Value proposeValue(uint32_t number) override;
};

struct BranchHIR : public HIR {
    BranchHIR(): HIR(kBranch) {}
    virtual ~BranchHIR() = default;

    int blockNumber;
    Value proposeValue(uint32_t number) override;
};

struct BranchIfTrueHIR : public HIR {
    BranchIfTrueHIR() = delete;
    BranchIfTrueHIR(std::pair<Value, Value> cond);
    virtual ~BranchIfTrueHIR() = default;

    std::pair<Value, Value> condition;
    int blockNumber;

    Value proposeValue(uint32_t number) override;
};

struct LabelHIR : public HIR {
    LabelHIR() = delete;
    LabelHIR(int blockNum): HIR(kLabel), blockNumber(blockNum) {}
    int blockNumber;
    std::vector<int> predecessors;
    std::vector<int> successors;
    std::list<std::unique_ptr<PhiHIR>> phis;

    Value proposeValue(uint32_t number) override;
};

struct DispatchSetupStackHIR : public HIR {
    DispatchSetupStackHIR() = delete;
    DispatchSetupStackHIR(std::pair<Value, Value> selector, int numArgs, int numKeyArgs);
    virtual ~DispatchSetupStackHIR() = default;

    std::pair<Value, Value> selectorValue;
    int numberOfArguments;
    int numberOfKeywordArguments;

    Value proposeValue(uint32_t number) override;
    int numberOfReservedRegisters() const override;
};

// Argument value and type is stored in |reads|
struct DispatchStoreArgHIR : public HIR {
    DispatchStoreArgHIR() = delete;
    DispatchStoreArgHIR(int argNum, std::pair<Value, Value> argVal);
    virtual ~DispatchStoreArgHIR() = default;

    int argumentNumber;
    std::pair<Value, Value> argumentValue;

    Value proposeValue(uint32_t number) override;
    int numberOfReservedRegisters() const override;
};

struct DispatchStoreKeyArgHIR : public HIR {
    DispatchStoreKeyArgHIR() = delete;
    DispatchStoreKeyArgHIR(int keyArgNum, std::pair<Value, Value> key, std::pair<Value, Value> keyVal);
    virtual ~DispatchStoreKeyArgHIR() = default;

    int keywordArgumentNumber;
    std::pair<Value, Value> keyword;
    std::pair<Value, Value> keywordValue;

    Value proposeValue(uint32_t number) override;
    int numberOfReservedRegisters() const override;
};

struct DispatchCallHIR : public HIR {
    DispatchCallHIR(): HIR(kDispatchCall) {}
    virtual ~DispatchCallHIR() = default;

    Value proposeValue(uint32_t number) override;
    int numberOfReservedRegisters() const override;
};

// TODO: Could make this "read" the return value of the DispatchCall, to make the dependency clear, although a bit
// redundant since Dispatches can't ever be culled due to possible side effects.
struct DispatchLoadReturnHIR : public HIR {
    DispatchLoadReturnHIR(): HIR(kDispatchLoadReturn) {}
    virtual ~DispatchLoadReturnHIR() = default;

    // Forces the kAny type for the return.
    Value proposeValue(uint32_t number) override;
};

struct DispatchLoadReturnTypeHIR : public HIR {
    DispatchLoadReturnTypeHIR(): HIR(kDispatchLoadReturnType) {}
    virtual ~DispatchLoadReturnTypeHIR() = default;

    Value proposeValue(uint32_t number) override;
};

struct DispatchCleanupHIR : public HIR {
    DispatchCleanupHIR(): HIR(kDispatchCleanup) {}
    virtual ~DispatchCleanupHIR() = default;

    // Always returns an invalid value, as this is a read-only operation.
    Value proposeValue(uint32_t number) override;
};

} // namespace hir

} // namespace hadron

#endif  // SRC_HADRON_HIR_HPP_