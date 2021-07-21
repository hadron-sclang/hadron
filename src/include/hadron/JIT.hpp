#ifndef SRC_INCLUDE_HADRON_JIT_HPP_
#define SRC_INCLUDE_HADRON_JIT_HPP_

#include <cstddef>
#include <memory>

namespace hadron {

class ErrorReporter;
struct Slot;

// TODO: refactor to represent a re-useable tool that produces some kind of build product, a callable block of code
// that is either "virtual" and so has no usable function pointers, or is rendered/realized and therefore has both a
// C entry pointer and a Hadron entry pointer. Then compiler context can produce build artifacts for consumption by
// the caller.

// Abstract base class for JIT compilation, allowing CodeGenerator to JIT to either virtual, testing or production
// backends.
// Steps to add a new instruction:
// 1) add a new abstract function here.
// 2) implement the new method in LightningJIT
// 3) implement the new method in VirtualJIT, including:
//    3a) add an enum value to Opcode
//    3b) implement the method in VirtualJIT
//    3c) add support for printing the opcode in VirtualJIT::toString()
// 4) add parsing for the opcode in main state machine in Assembler.rl, and a unit test in Assembler_unittests
// 5) add support for the opcode in MachineCodeRenderer::render()
class JIT {
public:
    JIT(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) {}
    JIT() = delete;
    virtual ~JIT() = default;

    // ===== JIT compilation
    virtual bool emit() = 0;
    virtual bool evaluate(Slot* value) const = 0;

    using Label = int32_t;
    using Reg = int32_t;
    using Address = void*;

    // Register number for the special Frame Pointer register.
    static constexpr Reg kFramePointerReg = -1;

    // ===== Machine Properties
    virtual int getRegisterCount() const = 0;
    virtual int getFloatRegisterCount() const = 0;

    // ===== Instruction Set (directly modeled from GNU Lightning instruction set, added as needed)
    // suffixes _i means 32-bit integer, _l means 64-bit integer, _w will call one of _i or _l depending on
    // word size of host machine.

    // * arithmetic
    // %target = %a + %b
    virtual void addr(Reg target, Reg a, Reg b) = 0;
    // %target = %a + b
    virtual void addi(Reg target, Reg a, int b) = 0;

    // * register setting
    // %target <- %value
    virtual void movr(Reg target, Reg value) = 0;
    // %target <- value
    virtual void movi(Reg target, int value) = 0;

    // * branches
    // if a >= b goto Label
    virtual Label bgei(Reg a, int b) = 0;
    // unconditionally jump to Label
    virtual Label jmpi() = 0;

    // * loads
    // %target = *(%address + offset)
    virtual void ldxi_w(Reg target, Reg address, int offset) = 0;
    virtual void ldxi_i(Reg target, Reg address, int offset) = 0;
    virtual void ldxi_l(Reg target, Reg address, int offset) = 0;

    // * stores
    // *address = value
    virtual void str_i(Reg address, Reg value) = 0;
    // *address = value
    virtual void sti_i(Address address, Reg value) = 0;
    // *(offset + address) = value  // note: immediate address with register offset not currently supported
    virtual void stxi_w(int offset, Reg address, Reg value) = 0;
    virtual void stxi_i(int offset, Reg address, Reg value) = 0;
    virtual void stxi_l(int offset, Reg address, Reg value) = 0;

    // * functions
    // mark the start of a new function
    virtual void prolog() = 0;
    // mark arguments for retrieval into registers later with getarg()
    virtual Label arg() = 0;
    // load argument into a register %target
    virtual void getarg_w(Reg target, Label arg) = 0;
    virtual void getarg_i(Reg target, Label arg) = 0;
    virtual void getarg_l(Reg target, Label arg) = 0;
    // allocate bytes on the stack, should be called after prolog() and before frame(). Note this API only allows for
    // one call to allocai per JIT instance but the underlying API is not so restrictive.
    virtual void allocai(int stackSizeBytes) = 0;
    // sets up a c-callabale stack frame of at least stackSizeBytes. Save all callee-save registers. The Size argument
    // needs to be at least as large as the sum of all calls to allocai.
    virtual void frame(int stackSizeBytes) = 0;
    // return with no value
    virtual void ret() = 0;
    // retr %r  (return value of reg r)
    virtual void retr(Reg r) = 0;
    // reti value (return immediate value)
    virtual void reti(int value) = 0;
    // mark the end of a function (should follow after any ret call)
    virtual void epilog() = 0;

    // * labels
    // Makes a new label for backward branches.
    virtual Label label() = 0;
    // Makes |target| point to |location|, for backward jumps.
    virtual void patchAt(Label target, Label location) = 0;
    // Makes |label| point to current position in JIT, for forward jumps.
    virtual void patch(Label label) = 0;

protected:
    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_JIT_HPP_