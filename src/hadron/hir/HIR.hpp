#ifndef SRC_HADRON_HIR_HIR_HPP_
#define SRC_HADRON_HIR_HIR_HPP_

#include "hadron/library/Symbol.hpp"
#include "hadron/lir/LIR.hpp"
#include "hadron/Type.hpp"

#include <list>
#include <unordered_set>
#include <vector>

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
    kAlias, // TODO

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

    // Given this HIR, and all other HIR |values| in the frame, output zero or more LIR instructions to |append|.
    virtual void lower(const std::vector<HIR*>& values, std::vector<lir::LIR*>& vRegs,
            std::list<std::unique_ptr<lir::LIR>>& append) const = 0;

    // Most HIR directly translates from NamedValue id to lir::VReg, but we introduce a function as a means of allowing
    // for HIR-specific changes to this.
    virtual lir::VReg vReg() const { return static_cast<lir::VReg>(value.id); }

protected:
    explicit HIR(Opcode op): opcode(op) {}
    HIR(Opcode op, Type typeFlags, library::Symbol valueName): opcode(op), value(kInvalidNVID, typeFlags, valueName) {}
};

} // namespace hir
} // namespace hadron

#endif  // SRC_HADRON_HIR_HPP_