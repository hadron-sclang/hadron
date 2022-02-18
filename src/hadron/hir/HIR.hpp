#ifndef SRC_HADRON_HIR_HIR_HPP_
#define SRC_HADRON_HIR_HIR_HPP_

#include "hadron/library/Symbol.hpp"
#include "hadron/lir/LIR.hpp"
#include "hadron/Type.hpp"

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
    NamedValue(): id(kInvalidNVID), typeFlags(Type::kNone), name() {}
    NamedValue(NVID nvid, Type flags, library::Symbol valueName): id(nvid), typeFlags(flags), name(valueName) {}
    NVID id;
    Type typeFlags;
    library::Symbol name; // Can be nil for anonymous values
};

enum Opcode {
    kBranch = 0,
    kBranchIfTrue = 1,
    kConstant = 2,
    kLoadArgument = 3,
    kLoadClassVariable = 4,
    kLoadInstanceVariable = 5,
    kMessage = 6,
    kMethodReturn = 7,
    kPhi = 8,
    kStoreClassVariable = 9,
    kStoreInstanceVariable = 10,
    kStoreReturn = 11
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
    virtual void lower(const std::vector<HIR*>& values, std::vector<LIRList::iterator>& vRegs,
            LIRList& append) const = 0;

    // Most HIR directly translates from NamedValue id to lir::VReg, but we introduce a function as a means of allowing
    // for HIR-specific changes to this.
    virtual lir::VReg vReg() const {
        return value.id != kInvalidNVID ? static_cast<lir::VReg>(value.id) : lir::kInvalidVReg;
    }

protected:
    explicit HIR(Opcode op): opcode(op) {}
    HIR(Opcode op, Type typeFlags, library::Symbol valueName): opcode(op), value(kInvalidNVID, typeFlags, valueName) {}
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_HPP_