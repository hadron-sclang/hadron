#ifndef SRC_INCLUDE_HADRON_JIT_HPP_
#define SRC_INCLUDE_HADRON_JIT_HPP_

#include "hadron/Slot.hpp"

#include <cstddef>
#include <memory>

namespace hadron {

// Abstract base class for JIT compilation, allowing CodeGenerator to JIT to either virtual, testing or production
// backends.
class JIT {
public:
    JIT() = default;
    virtual ~JIT() = default;

    // ===== JIT compilation
    virtual bool emit() = 0;
    virtual Slot value() = 0;

    using Label = int32_t;
    using Reg = int32_t;

    // ===== Machine Properties
    virtual int getRegisterCount() const = 0;
    virtual int getFloatRegisterCount() const = 0;

    // ===== Instruction Set (directly modeled from GNU Lightning instruction set, added as needed)

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

    // * stores
    // *(offset + address) = value
    virtual void stxi(int offset, Reg address, Reg value) = 0;

    // * functions
    // mark the start of a new function
    virtual void prolog() = 0;
    // mark arguments for retrieval into registers later with getarg()
    virtual Label arg() = 0;
    // load argument into a register %target
    virtual void getarg(Reg target, Label arg) = 0;
    // allocate bytes on the stack, should be called after prolog()
    virtual void allocai(int stackSizeBytes) = 0;
    // retr %r  (return value of reg r)
    virtual void retr(Reg r) = 0;
    // mark the end of a function (should follow after any ret call)
    virtual void epilog() = 0;

    // * labels
    // Makes a new label for backward branches.
    virtual Label label() = 0;
    // Makes |target| point to |location|, for backward jumps.
    virtual void patchAt(Label target, Label location) = 0;
    // Makes |label| point to current position in JIT, for forward jumps.
    virtual void patch(Label label) = 0;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_JIT_HPP_