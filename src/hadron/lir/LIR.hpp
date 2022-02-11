#ifndef SRC_HADRON_LIR_HPP_
#define SRC_HADRON_LIR_HPP_

#include "hadron/JIT.hpp"
#include "hadron/Type.hpp"

#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace hadron {
namespace lir {
using VReg = int32_t;
static constexpr int32_t kInvalidVReg = -1;
struct LIR;
} // namespace lir

using LIRList = std::list<std::unique_ptr<lir::LIR>>;

namespace lir {
enum Opcode {
    kLoadFromStack,
    kStoreToStack,
    kLoadFramePointer,
    kAdvanceFramePointer,
    kStoreToPointer,
    kLoadFromPointer,
    kJumpToPointer,
    kLoadConstant,


    // Slot unpacking and repacking. Unless type is certain at compile time values are generally packed.
    kUnpack,
    kPackFloat,
    kPackNil,
    kPackInt32,
    kPackBool,
    kPackPointer,
    kPackHash,
    kPackChar,

    kThrowException,

    kPhi,
    kLabel,
    kBranch,
    kBranchIfTrue,
    kBranchToRegister
};

struct LIR {
    LIR() = delete;
    virtual ~LIR() = default;

    Opcode opcode;
    VReg value;
    Type typeFlags;
    std::unordered_set<VReg> reads;

    // Built during register allocation, a map of all virtual registers in |arguments| and |vReg| to physical registers.
    std::unordered_map<VReg, JIT::Reg> valueLocations;

    // Due to register allocation and SSA form deconstruction any HIR operand may have a series of moves to and from
    // physical registers and/or spill storage. Record them here for scheduling later during machine code generation.
    // The keys are origins and values are destinations. Positive integers (and 0) indicate register numbers, and
    // negative values indicate spill slot indices, with spill slot 0 reserved for register move cycles. Move scheduling
    // requires origins be copied only once, so enforcing unique keys means trying to insert a move from an origin
    // already scheduled for a move is an error. These are *predicate* moves, meaning they are executed before the HIR.
    std::unordered_map<int, int> moves;

    // Emits machine code into the provided JIT buffer. Base implementation should be called first by derived classes,
    // and emits any needed register moves.
    virtual void emit(JIT* jit) const;

    // If true, the register allocation system will assume that this instruction destroys all register values during
    // execution, and will spill all outstanding register allocations. Typically used for message dispatch.
    virtual bool shouldPreserveRegisters() const { return false; }

protected:
    LIR(Opcode op, VReg v, Type t): opcode(op), value(v), typeFlags(t) {}
};

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_HPP_