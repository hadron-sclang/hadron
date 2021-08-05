#ifndef SRC_INCLUDE_HADRON_HIR_HPP_
#define SRC_INCLUDE_HADRON_HIR_HPP_

#include "hadron/Hash.hpp"
#include "hadron/Slot.hpp"

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

struct Block;

namespace hir {

enum Opcode {
    kLoadArgument,
    kConstant,
    kStoreReturn,

    // Method calling.
    kDispatchCall,  // save all registers, set up calling stack, represents a modification of the target
    kDispatchLoadReturn,  // just like LoadArgument, can get type or value from stack, call before Cleanup
    kDispatchCleanup, // must be called after a kDispatch
};

// All HIR instructions modify the value, thus creating a new version, and may read multiple other values, recorded in
// the reads member.
struct HIR {
    HIR() = delete;
    explicit HIR(Opcode op): opcode(op), valueNumber(-1) {}
    virtual ~HIR() = default;
    Opcode opcode;
    int32_t valueNumber;
    std::unordered_set<int32_t> reads;

    // Compare two HIR objects and return true if they are *semantically* the same.
    virtual bool isEquivalent(const HIR* hir) const = 0;
};

struct LoadArgumentHIR : public HIR {
    // If value is true this will load the value of the argument, if false it will load the type.
    LoadArgumentHIR(int32_t argIndex, bool value): HIR(kLoadArgument), index(argIndex), loadValue(value) {}
    virtual ~LoadArgumentHIR() = default;
    int32_t index;
    bool loadValue;

    bool isEquivalent(const HIR* hir) const override;
};

struct ConstantHIR : public HIR {
    ConstantHIR(const Slot& val): HIR(kConstant), value(val) {}
    virtual ~ConstantHIR() = default;
    Slot value;

    bool isEquivalent(const HIR* hir) const override;
};

struct StoreReturnHIR : public HIR {
    StoreReturnHIR(std::pair<int32_t, int32_t> retVal);
    virtual ~StoreReturnHIR() = default;
    std::pair<int32_t, int32_t> returnValue;

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
    void addKeywordArgument(std::pair<int32_t, int32_t> keyword, std::pair<int32_t, int32_t> value);
    void addArgument(std::pair<int32_t, int32_t> argument);

    std::vector<std::pair<int32_t, int32_t>> keywordArguments;
    std::vector<std::pair<int32_t, int32_t>> arguments;
};

struct DispatchLoadReturnHIR : public Dispatch {
    // If |value| is true this will load the value of the return, if false it will load the type.
    DispatchLoadReturnHIR(bool value): Dispatch(kDispatchLoadReturn), loadValue(value) {}
    virtual ~DispatchLoadReturnHIR() = default;
    bool loadValue;
};

struct DispatchCleanupHIR : public Dispatch {
    DispatchCleanupHIR(): Dispatch(kDispatchCleanup) {}
    virtual ~DispatchCleanupHIR() = default;
};

} // namespace hir

} // namespace hadron

#endif  // SRC_INCLUDE_HADRON_HIR_HPP_