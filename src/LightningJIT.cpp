#include "LightningJIT.hpp"

#include "runtime/Slot.hpp"
#include "SyntaxAnalyzer.hpp"

extern "C" {
#include "lightning.h"
}

namespace hadron {

class LightningJITBlock : public JITBlock {
public:
    LightningJITBlock(): m_blockEval(nullptr) {
        m_jitState = jit_new_state();
    }

    virtual ~LightningJITBlock() {
        _jit_destroy_state(m_jitState);
    }

    typedef void (*blockEval)(Slot*);

    Slot value() override {
        Slot result;
        m_blockEval(&result);
        return result;
    }

    jit_state_t* jitState() { return m_jitState; }

private:
    jit_state_t* m_jitState;
    blockEval m_blockEval;
};

} // namespace hadron


namespace {
void jitBlockInline(const hadron::ast::BlockAST* /* block */, hadron::LightningJITBlock* /* jit */) {
}
} // namespace

namespace hadron {

LightningJIT::LightningJIT() { }

LightningJIT::~LightningJIT() { }

// static
void LightningJIT::initJITGlobals() {
    init_jit(nullptr);
}

// static
void LightningJIT::finishJITGlobals() {
    finish_jit();
}

std::unique_ptr<JITBlock> LightningJIT::jitBlock(const ast::BlockAST* block) {
    auto jit = std::make_unique<LightningJITBlock>();
    _jit_prolog(jit->jitState());
    jitBlockInline(block, jit.get());
    _jit_epilog(jit->jitState());
    return jit;
}

} // namespace hadron