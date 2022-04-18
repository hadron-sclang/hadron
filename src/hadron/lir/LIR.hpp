#ifndef SRC_HADRON_LIR_HPP_
#define SRC_HADRON_LIR_HPP_

#include "hadron/JIT.hpp"
#include "hadron/Slot.hpp"

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {
namespace lir {
using VReg = int32_t;
static constexpr int32_t kInvalidVReg = -4;
static constexpr int32_t kContextPointerVReg = -3;
static constexpr int32_t kFramePointerVReg = -2;
static constexpr int32_t kStackPointerVReg = -1;
using LabelID = int32_t;
struct LIR;
} // namespace lir

using LIRList = std::list<std::unique_ptr<lir::LIR>>;

namespace lir {

enum Opcode {
    kAssign,
    kBranch,
    kBranchIfTrue,
    kBranchToRegister,
    kInterrupt,
    kLabel,
    kLoadConstant,
    kLoadFromPointer,
    kPhi,
    kStoreToPointer
};

struct LIR {
    LIR() = delete;
    virtual ~LIR() = default;

    Opcode opcode;
    VReg value; 
    TypeFlags typeFlags;
    std::unordered_set<VReg> reads;

    // Built during register allocation, a map of all virtual registers in |arguments| and |vReg| to physical registers.
    std::unordered_map<VReg, JIT::Reg> locations;

    // Due to register allocation and SSA form deconstruction any HIR operand may have a series of moves to and from
    // physical registers and/or spill storage. Record them here for scheduling later during machine code generation.
    // The keys are origins and values are destinations. Positive integers (and 0) indicate register numbers, and
    // negative values indicate spill slot indices, with spill slot 0 reserved for register move cycles. Move scheduling
    // requires origins be copied only once, so enforcing unique keys means trying to insert a move from an origin
    // already scheduled for a move is an error. These are *predicate* moves, meaning they are executed before the HIR.
    std::unordered_map<int, int> moves;

    inline JIT::Reg locate(VReg vReg) const {
        if (vReg >= 0) {
            assert(locations.find(vReg) != locations.end());
            return locations.at(vReg);
        }

        switch (vReg) {
        case kStackPointerVReg:
            return JIT::kStackPointerReg;

        case kFramePointerVReg:
            return JIT::kFramePointerReg;

        case kContextPointerVReg:
            return JIT::kContextPointerReg;
        }

        assert(false);
        return 0;
    }

    inline void read(VReg vReg) {
        assert(vReg != lir::kInvalidVReg);
        if (vReg >= 0) { reads.emplace(vReg); }
    }

    // If true, LinearBlock should assign a value to this LIR, otherwise it's assumed to be read-only.
    virtual bool producesValue() const { return false; }

    // If true, the register allocation system will assume that this instruction destroys all register values during
    // execution, and will spill all outstanding register allocations. Typically used for message dispatch.
    virtual bool shouldPreserveRegisters() const { return false; }

    // Emits machine code into the provided JIT buffer. Base implementation should be called first by derived classes,
    // and emits any needed register moves.
    virtual void emit(JIT* jit, std::vector<std::pair<JIT::Label, LabelID>>& patchNeeded) const = 0;

protected:
    LIR(Opcode op, TypeFlags t): opcode(op), value(kInvalidVReg), typeFlags(t) {}
    void emitBase(JIT* jit) const;
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_HPP_