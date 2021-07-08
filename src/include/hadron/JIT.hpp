#ifndef SRC_INCLUDE_HADRON_JIT_HPP_
#define SRC_INCLUDE_HADRON_JIT_HPP_

#include <cstddef>
#include <memory>

namespace hadron {

// Abstract base class for JIT compilation, allowing CodeGenerator to JIT to either testing or production backends.
class JIT {
public:
    JIT() = default;
    virtual ~JIT() = default;

    enum RegType {
        kSave,          // Saved during function calls (Lightning JIT_V() registers)
        kNoSave,        // Not saved during function calls (Lightning JIT_R() registers)
        kFloat          // Floating point registers (Lightning JIT_F() registers)
    };

    struct Reg {
        Reg(RegType t, int num): type(t), number(num) {}
        Reg() = delete;
        RegType type;
        int number;
    };

    using Label = size_t;

    // ===== Machine Properties
    virtual int getRegisterCount(RegType type) const = 0;

    // ===== Instruction Set (directly modeled from GNU Lightning instruction set, added as needed)

    // * arithmetic
    // % target = %a + %b
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

    // * functions
    // mark the start of a new function.
    virtual void prolog() = 0;
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