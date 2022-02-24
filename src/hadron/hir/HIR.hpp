#ifndef SRC_HADRON_HIR_HIR_HPP_
#define SRC_HADRON_HIR_HIR_HPP_

#include "hadron/library/Symbol.hpp"
#include "hadron/lir/LIR.hpp"
#include "hadron/Slot.hpp"

#include <cassert>
#include <list>
#include <unordered_set>
#include <vector>

namespace hadron {

struct Block;
struct Frame;

namespace hir {

using NVID = int32_t;
static constexpr int32_t kInvalidNVID = -1;
// This assumption has crept into the code so document it and enfore with the compiler.
static_assert(kInvalidNVID == lir::kInvalidVReg);

struct NamedValue {
    NamedValue(): id(kInvalidNVID), typeFlags(TypeFlags::kNoFlags), knownClassName(), name() {}
    NamedValue(NVID nvid, TypeFlags flags, library::Symbol valueName): id(nvid), typeFlags(flags),
            knownClassName(), name(valueName) {}
    NVID id;
    TypeFlags typeFlags;
    // TODO: this should probably be a std::unordered_set<>
    library::Symbol knownClassName; // If kObjectFlag is set and this is non-nil that means that if this value is an
                                    // object it can only be an object of the type named here.
    library::Symbol name; // Can be nil for anonymous values
};

enum Opcode {
    kBlockLiteral = 0,
    kBranch = 1,
    kBranchIfTrue = 2,
    kConstant = 3,
    kLoadArgument = 4,           // stack-relative
    kLoadFromPointer = 5,        // address known at compile time
    kLoadInstanceVariable = 6,   // this-relative
    kMessage = 7,
    kMethodReturn = 8,
    kPhi = 9,
    kStoreInstanceVariable = 10, // this-relative
    kStoreReturn = 11,           // stack-relative
    kStoreToPointer = 12         // address known at compile time
};

// All HIR instructions modify the value, thus creating a new version, and may read multiple other values, recorded in
// the reads member.
struct HIR {
    HIR() = delete;
    virtual ~HIR() = default;
    Opcode opcode;
    NamedValue value;

    std::unordered_set<NVID> reads;

    // Recommended way to set the id in |value| member. Allows the HIR object to modify the proposed value type. For
    // convenience returns |value| as recorded within this object. Can return an invalid value, which indicates
    // that this operation only consumes values but doesn't generate a new one.
    virtual NVID proposeValue(NVID id) = 0;

    // Given this HIR, and all other HIR |values| in the frame, output zero or more LIR instructions to |append|.
    virtual void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs,
            LIRList& append) const = 0;

    // Most HIR directly translates from NamedValue id to lir::VReg, but we introduce a function as a means of allowing
    // for HIR-specific changes to this.
    virtual lir::VReg vReg() const {
        return value.id != kInvalidNVID ? static_cast<lir::VReg>(value.id) : lir::kInvalidVReg;
    }

protected:
    explicit HIR(Opcode op): opcode(op) {}
    HIR(Opcode op, TypeFlags typeFlags, library::Symbol valueName): opcode(op),
            value(kInvalidNVID, typeFlags, valueName) {}
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_HPP_