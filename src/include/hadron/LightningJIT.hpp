#ifndef SRC_INCLUDE_HADRON_LIGHTNING_JIT_HPP_
#define SRC_INCLUDE_HADRON_LIGHTNING_JIT_HPP_

#include "hadron/JIT.hpp"

#include <vector>

// GNU Lightning external declarations.
extern "C" {
    struct jit_node;
    typedef struct jit_node jit_node_t;
    struct jit_state;
    typedef struct jit_state jit_state_t;
}

namespace hadron {

class LightningJIT : public JIT {
public:
    LightningJIT(std::shared_ptr<ErrorReporter> errorReporter);
    LightningJIT() = delete;
    virtual ~LightningJIT();

    bool emit() override;
    bool evaluate(Slot* value) const override;
    void print() const override;

    int getRegisterCount() const override;
    int getFloatRegisterCount() const override;

    void addr(Reg target, Reg a, Reg b) override;
    void addi(Reg target, Reg a, int b) override;
    void movr(Reg target, Reg value) override;
    void movi(Reg target, int value) override;
    Label bgei(Reg a, int b) override;
    Label jmpi() override;
    void ldxi(Reg target, Reg address, int offset) override;
    void str(Reg address, Reg value) override;
    void sti(Address address, Reg value) override;
    void stxi(int offset, Reg address, Reg value) override;
    void prolog() override;
    Label arg() override;
    void getarg(Reg target, Label arg) override;
    void allocai(int stackSizeBytes) override;
    void frame(int stackSizeBytes) override;
    void ret() override;
    void retr(Reg r) override;
    void reti(int value) override;
    void epilog() override;
    Label label() override;
    void patchAt(Label target, Label location) override;
    void patch(Label label) override;

    // GNU Lightning requires calls to global setup and teardown functions before emitting any JIT bytecode.
    static void initJITGlobals();
    static void finishJITGlobals();

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

} // namespace hadron

#endif