#ifndef SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_
#define SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_

#include "hadron/JIT.hpp"

#include <vector>

// Lightening external declarations.
extern "C" {
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
    void* address();

    // what does this return?
    size_t enterABI(size_t v, size_t vf);
    void leaveABI(size_t v, size_t vf);

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
    void prolog() override;  // rename to begin
    Label arg() override;
    void getarg_w(Reg target, Label arg) override;
    void getarg_i(Reg target, Label arg) override;
    void getarg_l(Reg target, Label arg) override;
    void allocai(int stackSizeBytes) override;
    void frame(int stackSizeBytes) override;
    void ret() override;
    void retr(Reg r) override;
    void reti(int value) override;
    void epilog() override;
    Label label() override;
    void patchThere(Label target, Label location) override;
    void patchHere(Label label) override;

    // Lightening requires a call to a global setup function before emitting any JIT bytecode.
    static void initJITGlobals();

private:
    // Converts flat register space to JIT_V and JIT_R equivalents.
    int reg(Reg r);
    jit_state_t* m_state;
    // Non-owning pointers to nodes within the jit_state struct, used for labels.
    std::vector<jit_node_t*> m_labels;
    // Offset in bytes from the stack frame pointer where the stack begins.
    int m_stackBase;
};

}

#endif // SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_