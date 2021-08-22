#ifndef SRC_INCLUDE_HADRON_HIR_HPP_
#define SRC_INCLUDE_HADRON_HIR_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {

// Represents a pair of value number (for Local Value Numbering during SSA form construction) and Type flags created
// by ORing the Types of variables together across Phi nodes.
struct Value {
    // A typeFlag of 0 represents an invalid Value, as does a value number of 0.
    Value(): number(0), typeFlags(0) {}
    Value(uint32_t num, uint32_t flags): number(num), typeFlags(flags) {}
    ~Value() = default;
    bool isValid() const { return number != 0 && typeFlags != 0; }
    bool operator==(const Value& v) const { return number == v.number; }
    bool operator!=(const Value& v) const { return number != v.number; }

    uint32_t number;
    uint32_t typeFlags;
};

} // namespace hadron

// Inject a hash template specialization for Value into the std namespace, so we can use Values as keys in STL
// maps and sets. Note that we only key off the number in a Value.
namespace std {
template<> struct hash<hadron::Value> {
    std::size_t operator()(const hadron::Value & v) const noexcept {
        return std::hash<uint32_t>{}(v.number);
    }
};
} // namespace std

namespace hadron {

struct Block;
struct Frame;

namespace hir {

enum Opcode {
    kLoadArgument,
    kLoadArgumentType,
    kConstant,
    kStoreReturn,
    kResolveType, // Many other operations (such as binops and dispatch) require runtime knowledge of the type of the
                  // input value. If it's known at compile time we can replace this with a ConstantHIR opcode.
                  // If unknown this adds the type as a value (with type kType) that can be manipulated like any other
                  // value.

    // Control flow
    kPhi,
    kBranch,
    kBranchIfZero,

    // For Linear HIR represents the start of a block as well as a container for any phis at the start of the block.
    kLabel,
    // For shifting of values between blocks or in and out of spill storage.
    kScheduleMoves,

    // Method calling.
    kDispatchCall,  // save all registers, set up calling stack, represents a modification of the target
    kDispatchLoadReturn,  // just like LoadArgument, can get type or value from stack, call before Cleanup
    kDispatchLoadReturnType,
    kDispatchCleanup, // must be called after a kDispatch
};

// All HIR instructions modify the value, thus creating a new version, and may read multiple other values, recorded in
// the reads member.
struct HIR {
    HIR() = delete;
    explicit HIR(Opcode op): opcode(op) {}
    virtual ~HIR() = default;
    Opcode opcode;
    Value value;
    std::unordered_set<Value> reads;
    // Due to register allocation and SSA form deconstruction every HIR operand may have a series of moves to and from
    // physical registers and/or spill storage. Record them here for scheduling later during machine code generation.
    // The keys are origins and values are destinations. Positive integers (and 0) indicate register numbers, and
    // negative values indicate spill slot indices offset by 1 (meaning -1 means spill slot 0, -2 means 1, etc).
    // Move scheduling requires origins be copied only once, so enforcing unique keys means trying to insert a move from
    // an origin already scheduled for a move is an error.
    std::unordered_map<int, int> moves;

    // Recommended way to set the |value| member. Allows the HIR object to modify the proposed value type. For
    // convenience returns |value| as recorded within this object. Can return an invalid value, which indicates
    // that this operation is read-only.
    virtual Value proposeValue(uint32_t number) = 0;
    // Compare two HIR objects and return true if they are *semantically* the same.
    virtual bool isEquivalent(const HIR* hir) const = 0;
};

// Loads the argument at |index| from the stack.
struct LoadArgumentHIR : public HIR {
    LoadArgumentHIR(Frame* f, int argIndex): HIR(kLoadArgument), frame(f), index(argIndex) {}
    virtual ~LoadArgumentHIR() = default;
    Frame* frame;
    int index;

    // Forces the kAny type for all arguments.
    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

// Represents the type associated with the value at |index|.
struct LoadArgumentTypeHIR : public HIR {
    LoadArgumentTypeHIR(Frame* f, int argIndex): HIR(kLoadArgumentType), frame(f), index(argIndex) {}
    virtual ~LoadArgumentTypeHIR() = default;
    Frame* frame;
    int index;

    // Forces the kType type for all arguments.
    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct ConstantHIR : public HIR {
    ConstantHIR(const Slot& c): HIR(kConstant), constant(c) {}
    virtual ~ConstantHIR() = default;
    Slot constant;

    // Forces the type of the |constant| Slot.
    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct StoreReturnHIR : public HIR {
    StoreReturnHIR(Frame* f, std::pair<Value, Value> retVal);
    virtual ~StoreReturnHIR() = default;
    Frame* frame;
    std::pair<Value, Value> returnValue;

    // Always returns an invalid value, as this is a read-only operation.
    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct ResolveTypeHIR : public HIR {
    // Note that typeOfValue is not added to |reads|, as this doesn't require read access to the typeOfValue. This HIR
    // represents a note to the compiler that for dynamic type variables the type must be tracked in a separate register
    // from the value.
    ResolveTypeHIR(Value v): HIR(kResolveType), typeOfValue(v) {}
    virtual ~ResolveTypeHIR() = default;
    // ResolveType returns as a value the type of this value.
    Value typeOfValue;

    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct PhiHIR : public HIR {
    PhiHIR(): HIR(kPhi) {}

    std::vector<Value> inputs;
    void addInput(Value v);
    // A phi is *trivial* if it has only one distinct input value that is not self-referential. If this phi is trivial,
    // return the trivial value. Otherwise return an invalid value.
    Value getTrivialValue() const;

    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct BranchHIR : public HIR {
    BranchHIR(): HIR(kBranch) {}
    virtual ~BranchHIR() = default;

    int blockNumber;
    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct BranchIfZeroHIR : public HIR {
    BranchIfZeroHIR() = delete;
    BranchIfZeroHIR(std::pair<Value, Value> cond);
    virtual ~BranchIfZeroHIR() = default;

    std::pair<Value, Value> condition;
    int blockNumber;

    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct LabelHIR : public HIR {
    LabelHIR() = delete;
    LabelHIR(int blockNum): HIR(kLabel), blockNumber(blockNum) {}
    int blockNumber;
    std::vector<int> predecessors;
    std::vector<int> successors;
    std::list<std::unique_ptr<PhiHIR>> phis;
    std::unordered_set<size_t> liveIns;

    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

// Private struct to provide the overrides around equivalence, which are all the same for the Dispatch HIR objects.
struct Dispatch : public HIR {
    Dispatch() = delete;
    virtual ~Dispatch() = default;

    bool isEquivalent(const HIR* hir) const override;

protected:
    Dispatch(Opcode op): HIR(op) {}
};

struct DispatchCallHIR : public Dispatch {
    DispatchCallHIR(): Dispatch(kDispatchCall) {}
    virtual ~DispatchCallHIR() = default;

    // Convenience routines that add to respective vectors but also add to the <reads> structure.
    void addKeywordArgument(std::pair<Value, Value> key, std::pair<Value, Value> keyValue);
    void addArgument(std::pair<Value, Value> argument);

    std::vector<Value> keywordArguments;
    std::vector<Value> arguments;

    // Copies the type of its first argument, which is the this pointer for the dispatch, and doesn't change.
    Value proposeValue(uint32_t number) override;
};

// TODO: Could make this "read" the return value of the DispatchCall, to make the dependency clear, although a bit
// redundant since Dispatches can't ever be culled due to possible side effects.
struct DispatchLoadReturnHIR : public Dispatch {
    DispatchLoadReturnHIR(): Dispatch(kDispatchLoadReturn) {}
    virtual ~DispatchLoadReturnHIR() = default;

    // Forces the kAny type for the return.
    Value proposeValue(uint32_t number) override;
};

struct DispatchLoadReturnTypeHIR : public Dispatch {
    DispatchLoadReturnTypeHIR(): Dispatch(kDispatchLoadReturnType) {}
    virtual ~DispatchLoadReturnTypeHIR() = default;

    Value proposeValue(uint32_t number) override;
};

struct DispatchCleanupHIR : public Dispatch {
    DispatchCleanupHIR(): Dispatch(kDispatchCleanup) {}
    virtual ~DispatchCleanupHIR() = default;

    // Always returns an invalid value, as this is a read-only operation.
    Value proposeValue(uint32_t number) override;
};

} // namespace hir

} // namespace hadron

#endif  // SRC_INCLUDE_HADRON_HIR_HPP_