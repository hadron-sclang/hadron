#ifndef SRC_HADRON_HIR_HIR_HPP_
#define SRC_HADRON_HIR_HIR_HPP_

#include "hadron/library/Symbol.hpp"
#include "hadron/Type.hpp"

#include <unordered_set>

namespace hadron {

struct Block;
struct Frame;

namespace hir {

using NVID = int32_t;
static constexpr int32_t kInvalidNVID = -1;

struct NamedValue {
    NamedValue(): id(kInvalidNVID), typeFlags(Type::kNone), name() {}
    NamedValue(NVID nvid, Type flags, library::Symbol valueName): id(nvid), typeFlags(flags), name(valueName) {}
    NVID id;
    Type typeFlags;
    library::Symbol name; // Can be nil for anonymous values
};

enum Opcode {
    kLoadArgument,
    kConstant,
    kMethodReturn,

    // Class state changes
    kLoadInstanceVariable,
    kLoadClassVariable,
    kStoreInstanceVariable,
    kStoreClassVariable,

    // Control flow
    kPhi,
    kBranch,
    kBranchIfTrue,

    // SC is a heavily message-based language, and HIR treats almost all operations as messages. There's lots of options
    // for optimization for messages, mostly inlining, that can happen on lowering MessageHIR to LIR. But, because of
    // the diversity of ways to pass message in LSC, we route everything through MessageHIR first, to manage inlining in
    // a central point inside of MessageHIR.
    kMessage
};

// All HIR instructions modify the value, thus creating a new version, and may read multiple other values, recorded in
// the reads member.
struct HIR {
    HIR() = delete;
    virtual ~HIR() = default;
    Opcode opcode;
    NamedValue value;

    std::unordered_set<NVID> reads;

    // Recommended way to set the id  in |value| member. Allows the HIR object to modify the proposed value type. For
    // convenience returns |value| as recorded within this object. Can return an invalid value, which indicates
    // that this operation only consumes values but doesn't generate a new one.
    virtual NVID proposeValue(NVID id) = 0;

protected:
    explicit HIR(Opcode op): opcode(op) {}
    HIR(Opcode op, Type typeFlags, library::Symbol valueName): opcode(op), value(kInvalidNVID, typeFlags, valueName) {}
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