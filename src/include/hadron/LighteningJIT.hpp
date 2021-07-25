#ifndef SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_
#define SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_

#include "hadron/JIT.hpp"

#include <vector>

// Lightening external declarations.
extern "C" {
struct jit_gpr;
typedef struct jit_gpr jit_gpr_t;
struct jit_pointer;
typedef struct jit_pointer jit_pointer_t;
struct jit_state;
typedef struct jit_state jit_state_t;
}

namespace hadron {

class LighteningJIT : public JIT {
public:
    LighteningJIT(std::shared_ptr<ErrorReporter> errorReporter);
    LighteningJIT() = delete;
    virtual ~LighteningJIT();

    static bool markThreadForJITCompilation();
    static void markThreadForJITExecution();

    // Call this stuff from Compiler, which knows it has a LighteningJIT.
    // Begin recording jit bytecode into the provideed buffer, of maximum size.
    void begin(uint8_t* buffer, size_t size);
    // returns true if the bytecode didn't fit into the provided buffer size.
    bool hasJITBufferOverflow();
    // Set pointer back to beginning of buffer.
    void reset();
    // End recording jit bytecode into the buffer, writes the final size to sizeOut. Returns the jit address to begin().
    void* end(size_t* sizeOut);
    // Get the current address of the JIT.
    uint8_t* address();

    // Save current state from the calling C-style stack frame, including all callee-save registers, and update the
    // C stack pointer (modulo alignment) to point just below this. Returns the number of bytes pushed on to the stack,
    // which should be passed back to leaveABI() as the stackSize argument to restore the stack to original state.
    size_t enterABI();
    // Load 2 arguments from C calling code stack frame or registers, and move them into the supplied reg arguments.
    void loadCArgs2(Reg arg1, Reg arg2);
    // Computes JIT_SP-2 and returns.
    Reg getCStackPointerRegister() const;
    void leaveABI(size_t stackSize);

    // ==== JIT overrides
    int getRegisterCount() const override;
    int getFloatRegisterCount() const override;

    void addr(Reg target, Reg a, Reg b) override;
    void addi(Reg target, Reg a, int b) override;
    void movr(Reg target, Reg value) override;
    void movi(Reg target, int value) override;
    Label bgei(Reg a, int b) override;
    Label jmpi() override;
    void ldxi_w(Reg target, Reg address, int offset) override;
    void ldxi_i(Reg target, Reg address, int offset) override;
    void ldxi_l(Reg target, Reg address, int offset) override;
    void str_i(Reg address, Reg value) override;
    void sti_i(Address address, Reg value) override;
    void stxi_w(int offset, Reg address, Reg value) override;
    void stxi_i(int offset, Reg address, Reg value) override;
    void stxi_l(int offset, Reg address, Reg value) override;
    void ret() override;
    void retr(Reg r) override;
    void reti(int value) override;
    Label label() override;
    void patchHere(Label label) override;
    void patchThere(Label target, Label location) override;

    // Lightening requires a call to a global setup function before emitting any JIT bytecode.
    static void initJITGlobals();

private:
    // We need to save all of the callee-save registers, which is a per-architecture value not exposed by lightening.h
    // so supplied here.
#   if defined(__i386__)
        static constexpr size_t kCalleeSaveRegisters = 3;
#   elif defined(__x86_64__)
        static constexpr size_t kCalleeSaveRegisters = 7;
#   elif defined(__arm__)
        static constexpr size_t kCalleeSaveRegisters = 7;
#   elif defined(__aarch64__)
        static constexpr size_t kCalleeSaveRegisters = 10;
#   else
#   error "Undefined chipset"
#   endif

    // Converts register number to the Lightening register type.
    jit_gpr_t reg(Reg r);
    jit_state_t* m_state;
    // Non-owning pointers to nodes within the jit_state struct, used for labels.
    std::vector<jit_pointer_t> m_labels;
    // Offset in bytes from the stack frame pointer where the stack begins.
    int m_stackBase;
};

}

#endif // SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_