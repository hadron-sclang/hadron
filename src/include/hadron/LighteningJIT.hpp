#ifndef SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_
#define SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_

#include "hadron/JIT.hpp"

#include <vector>

// Lightening external declarations.
extern "C" {
    struct jit_node;
    typedef struct jit_node jit_node_t;
    struct jit_state;
    typedef struct jit_state jit_state_t;
}

namespace hadron {

class LighteningJIT : public JIT {
public:
    LighteningJIT(std::shared_ptr<ErrorReporter> errorReporter);
    LighteningJIT() = delete;
    virtual ~LighteningJIT();

    bool emit() override;
    bool evaluate(Slot* value) const override;

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
    void patchAt(Label target, Label location) override;
    void patch(Label label) override;

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

    typedef int (*Value)(Slot* returnSlot);
    Value m_jit;
};

}

#endif // SRC_INCLUDE_HADRON_LIGHTENING_JIT_HPP_