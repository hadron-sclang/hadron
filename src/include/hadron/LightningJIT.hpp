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
    LightningJIT();
    virtual ~LightningJIT();

    int getRegisterCount(RegType type) const override;

    void addr(Reg target, Reg a, Reg b) override;
    void addi(Reg target, Reg a, int b) override;
    void movr(Reg target, Reg value) override;
    void movi(Reg target, int value) override;
    Label bgei(Reg a, int b) override;
    Label jmpi() override;
    void prolog() override;
    void retr(Reg r) override;
    void epilog() override;
    Label label() override;
    void patchAt(Label target, Label location) override;
    void patch(Label label) override;

    // GNU Lightning requires calls to global setup and teardown functions before emitting any JIT bytecode.
    static void initJITGlobals();
    static void finishJITGlobals();

private:
    int reg(Reg r);
    jit_state_t* m_state;
    // non-owning pointers
    std::vector<jit_node_t*> m_labels;
};

} // namespace hadron

#endif