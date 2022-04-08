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

class Block;
struct Frame;
struct LinearFrame;

namespace hir {

using ID = int32_t;
static constexpr int32_t kInvalidID = -1;
// This assumption has crept into the code so document it and enforce with the compiler.
static_assert(kInvalidID == lir::kInvalidVReg);

enum Opcode {
    kBlockLiteral,
    kBranch,
    kBranchIfTrue,
    kConstant,
    kLoadOuterFrame,
    kMessage,
    kMethodReturn,
    kPhi,
    kReadFromClass,
    kReadFromContext,
    kReadFromFrame,
    kReadFromThis,
    kRouteToSuperclass,
    kStoreReturn,
    kWriteToClass,
    kWriteToFrame,
    kWriteToThis
};

// All HIR instructions modify the value, thus creating a new version, and may read multiple other values, recorded in
// the reads member.
struct HIR {
    HIR() = delete;
    virtual ~HIR() = default;
    Opcode opcode;

    ID id;
    TypeFlags typeFlags;

    // The set of NVIDs that this HIR uses as inputs.
    std::unordered_set<ID> reads;

    // The set of HIR that consume the output of this HIR.
    std::unordered_set<HIR*> consumers;
    Block* owningBlock;

    // Recommended way to set the id in |value| member. Allows the HIR object to modify the proposed value type. For
    // convenience returns |value| as recorded within this object. Can return an invalid value, which indicates
    // that this operation only consumes values but doesn't generate a new one.
    virtual ID proposeValue(ID proposedId) = 0;

    // Replace all references to |original| with |replacement| for this instruction. Returns true if this resulted in
    // any change to the HIR.
    virtual bool replaceInput(ID original, ID replacement) = 0;

    // Given this HIR, and all other HIR |values| in the frame, output zero or more LIR instructions to |linearFrame|.
    virtual void lower(const std::vector<HIR*>& values, LinearFrame* linearFrame) const = 0;

    // Most HIR directly translates from NamedValue id to lir::VReg, but we introduce a function as a means of allowing
    // for HIR-specific changes to this.
    virtual lir::VReg vReg() const {
        return id != kInvalidID ? static_cast<lir::VReg>(id) : lir::kInvalidVReg;
    }

protected:
    explicit HIR(Opcode op): opcode(op), id(kInvalidID), typeFlags(TypeFlags::kNoFlags), owningBlock(nullptr) {}
    HIR(Opcode op, TypeFlags flags): opcode(op), id(kInvalidID), typeFlags(flags), owningBlock(nullptr) {}

    // Used by derived classes in replaceInput() calls. Updates the |reads| set. Returns true if a swap occured.
    inline bool replaceReads(ID original, ID replacement) {
        if (reads.erase(original)) {
            reads.emplace(replacement);
            return true;
        }
        return false;
    }
};

} // namespace hir
} // namespace hadron

#endif // SRC_HADRON_HIR_HPP_