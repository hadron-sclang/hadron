#include "hadron/LightningJIT.hpp"

extern "C" {
#include "lightning.h"
}

namespace hadron {

LightningJIT::LightningJIT(): m_stackBase(0) {
    m_state = jit_new_state();
}

LightningJIT::~LightningJIT() {
    _jit_destroy_state(m_state);
}

bool LightningJIT::emit() {
    m_jit = reinterpret_cast<Value>(_jit_emit(m_state));
    return (m_jit != nullptr);
}

Slot LightningJIT::value() {
    Slot returnSlot;
    m_jit(&returnSlot);
    return returnSlot;
}

int LightningJIT::getRegisterCount(RegType type) const {
    switch (type) {
    case JIT::kSave:
        return jit_v_num();

    case JIT::kNoSave:
        return jit_r_num();

    case JIT::kFloat:
        return jit_f_num();
    }
}

void LightningJIT::addr(Reg target, Reg a, Reg b) {
    _jit_new_node_www(m_state, jit_code_addr, reg(target), reg(a), reg(b));
}

void LightningJIT::addi(Reg target, Reg a, int b) {
    _jit_new_node_www(m_state, jit_code_addi, reg(target), reg(a), b);
}

void LightningJIT::movr(Reg target, Reg value) {
    if (target != value) {
        _jit_new_node_ww(m_state, jit_code_movr, reg(target), reg(value));
    }
}

void LightningJIT::movi(Reg target, int value) {
    _jit_new_node_ww(m_state, jit_code_movi, reg(target), value);
}

JIT::Label LightningJIT::bgei(Reg a, int b) {
    m_labels.emplace_back(_jit_new_node_pww(m_state, jit_code_bgei, nullptr, reg(a), b));
    return m_labels.size();
}

JIT::Label LightningJIT::jmpi() {
    m_labels.emplace_back(_jit_new_node_p(m_state, jit_code_jmpi, nullptr));
    return m_labels.size();
}

void LightningJIT::stxi(int offset, Reg address, Reg value) {
    _jit_new_node_www(m_state, jit_code_stxi_i, offset, reg(address), reg(value));
}

void LightningJIT::prolog() {
    _jit_prolog(m_state);
}

JIT::Label LightningJIT::arg() {
    m_labels.emplace_back(_jit_arg(m_state));
    return m_labels.size();
}

void LightningJIT::getarg(Reg target, Label arg) {
    _jit_getarg_i(m_state, reg(target), m_labels[arg]);
}

void LightningJIT::allocai(int stackSizeBytes) {
    m_stackBase = _jit_allocai(m_state, stackSizeBytes);
}

void LightningJIT::retr(Reg r) {
    _jit_retr(m_state, reg(r));
}

void LightningJIT::epilog() {
    _jit_epilog(m_state);
}

JIT::Label LightningJIT::label() {
    m_labels.emplace_back(_jit_label(m_state));
    return m_labels.size();
}

void LightningJIT::patchAt(Label target, Label location) {
    _jit_patch_at(m_state, m_labels[target], m_labels[location]);
}

void LightningJIT::patch(Label label) {
    _jit_patch(m_state, m_labels[label]);
}

// static
void LightningJIT::initJITGlobals() {
    init_jit(nullptr);
}

// static
void LightningJIT::finishJITGlobals() {
    finish_jit();
}

int LightningJIT::reg(Reg r) {
    switch (r.type) {
    case JIT::kSave:
        return JIT_V(r.number);

    case JIT::kNoSave:
        return JIT_R(r.number);

    case JIT::kFloat:
        return JIT_F(r.number);
    }
}

} // namespace hadron