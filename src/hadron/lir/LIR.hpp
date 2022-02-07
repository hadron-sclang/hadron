#ifndef SRC_HADRON_LIR_HPP_
#define SRC_HADRON_LIR_HPP_

#include "hadron/JIT.hpp"

#include <unordered_map>
#include <vector>

namespace hadron {
namespace lir {

using VReg = int32_t;
static constexpr int32_t kInvalidVReg = -1;

enum Opcode {
    kLoadSlotFromStack,
    kStoreSlotToStack,

    kLabel
};

struct LIR {
    LIR() = delete;
    virtual ~LIR() = default;

    Opcode opcode;
    VReg vReg;
    std::vector<VReg> arguments;

    // Built during register allocation, a map of all virtual registers in |arguments| and |vReg| to physical registers.
    std::unordered_map<VReg, JIT::Reg> valueLocations;

    // Due to register allocation and SSA form deconstruction any HIR operand may have a series of moves to and from
    // physical registers and/or spill storage. Record them here for scheduling later during machine code generation.
    // The keys are origins and values are destinations. Positive integers (and 0) indicate register numbers, and
    // negative values indicate spill slot indices, with spill slot 0 reserved for register move cycles. Move scheduling
    // requires origins be copied only once, so enforcing unique keys means trying to insert a move from an origin
    // already scheduled for a move is an error. These are *predicate* moves, meaning they are executed before the HIR.
    std::unordered_map<int, int> moves;

    // Returns how many additional registers this LIR will need. A negative value means that all registers should be
    // reserved, typically used for register preservation during a function call. Reserved registers are allocated from
    // the highest number down, in the hopes they will not interfere with value register allocation, which starts at
    // register 0. Default implementation returns 0.
    virtual int numberOfReservedRegisters() const;

protected:
    explicit LIR(Opcode op): opcode(op) {}
};

/*

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
*/

} // namespace lir
} // namespace hadron

#endif // SRC_HADRON_LIR_HPP_