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

// The literature calls the entire data structure a Control Flow Graph, and the individual nodes are called Blocks.
// There is also a strict heirarchy of scope, which the LSC parser at least calls a Block. Could be called a Frame,
// since it will most definitely correspond to a stack frame. Frames are where local (read: stack-based) variables
// live, and Frames are nestable.

// So the plan is to follow Braun 2013 to build the SSA directly from the parse tree, skipping AST construction.
// This takes a few passes to clean up and get to minimal pruned SSA form. Type computations follow along for the
// ride as parallel variables to their named values. We perform LVN and constant folding as we go.

// We can then do a bottoms-up dead code elimination pass. This allows us to be verbose with the type system
// assignments, I think, as we can treat them like parallel values.
namespace hadron {


// Represents a pair of value number (for Local Value Numbering during SSA form construction) and Type flags created
// by ORing the Types of variables together across Phi nodes.
struct Value {
    // A typeFlag of 0 represents an invalid Value.
    Value(): number(0), typeFlags(0) {}
    Value(uint32_t num, uint32_t flags): number(num), typeFlags(flags) {}
    ~Value() = default;
    bool isValid() const { return typeFlags != 0; }
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
    kConstant,
    kStoreReturn,

    // Control flow
    kPhi,
    kIf,

    // Method calling.
    kDispatchCall,  // save all registers, set up calling stack, represents a modification of the target
    kDispatchLoadReturn,  // just like LoadArgument, can get type or value from stack, call before Cleanup
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

struct ConstantHIR : public HIR {
    ConstantHIR(const Slot& c): HIR(kConstant), constant(c) {}
    virtual ~ConstantHIR() = default;
    Slot constant;

    // Forces the type of the |constant| Slot.
    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

struct StoreReturnHIR : public HIR {
    StoreReturnHIR(Frame* f, Value retVal);
    virtual ~StoreReturnHIR() = default;
    Frame* frame;
    Value returnValue;

    // Always returns an invalid value, as this is a read-only operation.
    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};


struct PhiHIR : public HIR {
    PhiHIR(): HIR(kPhi) {}

    std::vector<Value> inputs;
    void addInput(Value v);

    Value proposeValue(uint32_t number) override;
    bool isEquivalent(const HIR* hir) const override;
};

// TODO: inherit from some common BranchHIR terminal instruction? Maybe also a block exit HIR too?
struct IfHIR : public HIR {
    IfHIR() = delete;
    IfHIR(Value cond);
    virtual ~IfHIR() = default;

    Value condition;
    int trueBlock;
    int falseBlock; // can be -1?

    // Computation of the type of an if is a function of the type of the branching blocks, so for initial value
    // type computation we return kAny because the types of the branch blocks aren't yet known. A subsequent round
    // of phi reduction/constant folding/dead code elimination could simplify the type considerably.
    Value proposeValue(uint32_t number) override;
    // Because 'if' statements are terminal blocks, isEquivalent is always false.
    bool isEquivalent(const HIR* hir) const override;
};

// Private struct to provide the overrides around equivalence, which are all the same for the Dispatch HIR objects.
struct Dispatch : public HIR {
    virtual ~Dispatch() = default;

    bool isEquivalent(const HIR* hir) const override;

protected:
    Dispatch(Opcode op): HIR(op) {}
};

struct DispatchCallHIR : public Dispatch {
    DispatchCallHIR(): Dispatch(kDispatchCall) {}
    virtual ~DispatchCallHIR() = default;

    // Convenience routines that add to respective vectors but also add to the <reads> structure.
    void addKeywordArgument(Value key, Value keyValue);
    void addArgument(Value argument);

    std::vector<Value> keywordArguments;
    std::vector<Value> arguments;

    // Copies the type of its first argument, which is the this pointer for the dispatch, and doesn't change.
    Value proposeValue(uint32_t number) override;
};

// TODO: Could make this "read" the return value of the DispatchCall, to make the dependency clear
struct DispatchLoadReturnHIR : public Dispatch {
    DispatchLoadReturnHIR(): Dispatch(kDispatchLoadReturn) {}
    virtual ~DispatchLoadReturnHIR() = default;

    // Forces the kAny type for the return.
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