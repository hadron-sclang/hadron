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

int LightningJIT::getRegisterCount() const {
    return jit_v_num() + jit_r_num();
}

int LightningJIT::getFloatRegisterCount() const {
    return jit_f_num();
}

void LightningJIT::addr(Reg target, Reg a, Reg b) {
    _jit_new_node_www(m_state, jit_code_addr, target, a, b);
}

void LightningJIT::addi(Reg target, Reg a, int b) {
    _jit_new_node_www(m_state, jit_code_addi, target, a, b);
}

void LightningJIT::movr(Reg target, Reg value) {
    if (target != value) {
        _jit_new_node_ww(m_state, jit_code_movr, target, value);
    }
}

void LightningJIT::movi(Reg target, int value) {
    _jit_new_node_ww(m_state, jit_code_movi, target, value);
}

JIT::Label LightningJIT::bgei(Reg a, int b) {
    m_labels.emplace_back(_jit_new_node_pww(m_state, jit_code_bgei, nullptr, a, b));
    return m_labels.size() - 1;
}

JIT::Label LightningJIT::jmpi() {
    m_labels.emplace_back(_jit_new_node_p(m_state, jit_code_jmpi, nullptr));
    return m_labels.size() - 1;
}

void LightningJIT::str(Reg address, Reg value) {
    _jit_new_node_ww(m_state, jit_code_str_i, address, value);
}

void LightningJIT::sti(Address address, Reg value) {
    _jit_new_node_pw(m_state, jit_code_sti_i, address, value);
}

void LightningJIT::stxi(int offset, Reg address, Reg value) {
    _jit_new_node_www(m_state, jit_code_stxi_i, offset, address, value);
}

void LightningJIT::prolog() {
    _jit_prolog(m_state);
}

JIT::Label LightningJIT::arg() {
    m_labels.emplace_back(_jit_arg(m_state));
    return m_labels.size() - 1;
}

void LightningJIT::getarg(Reg target, Label arg) {
    _jit_getarg_i(m_state, target, m_labels[arg]);
}

void LightningJIT::allocai(int stackSizeBytes) {
    m_stackBase = _jit_allocai(m_state, stackSizeBytes);
}

void LightningJIT::ret() {
    _jit_ret(m_state);
}

void LightningJIT::retr(Reg r) {
    _jit_retr(m_state, r);
}

void LightningJIT::epilog() {
    _jit_epilog(m_state);
}

JIT::Label LightningJIT::label() {
    m_labels.emplace_back(_jit_label(m_state));
    return m_labels.size() - 1;
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

} // namespace hadron