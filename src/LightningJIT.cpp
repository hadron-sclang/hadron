#include "hadron/LightningJIT.hpp"

#include "hadron/ErrorReporter.hpp"

#include "fmt/format.h"

extern "C" {
#include "lightning.h"
}

namespace hadron {

LightningJIT::LightningJIT(std::shared_ptr<ErrorReporter> errorReporter):
    JIT(errorReporter),
    m_stackBase(0) {
    m_state = jit_new_state();
}

LightningJIT::~LightningJIT() {
    _jit_destroy_state(m_state);
}

bool LightningJIT::emit() {
    m_jit = reinterpret_cast<Value>(_jit_emit(m_state));
    return (m_jit != nullptr);
}

bool LightningJIT::evaluate(Slot* value) const {
    return m_jit(value) != 0;
}

void LightningJIT::print() const {
    _jit_print(m_state);
    _jit_clear_state(m_state);
}

int LightningJIT::getRegisterCount() const {
    return JIT_R_NUM + JIT_V_NUM;
}

int LightningJIT::getFloatRegisterCount() const {
    return JIT_F_NUM;
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
    return m_labels.size() - 1;
}

JIT::Label LightningJIT::jmpi() {
    m_labels.emplace_back(_jit_new_node_p(m_state, jit_code_jmpi, nullptr));
    return m_labels.size() - 1;
}

void LightningJIT::ldxi(Reg target, Reg address, int offset) {
    _jit_new_node_www(m_state, jit_code_ldxi_i, reg(target), reg(address), offset);
}

void LightningJIT::str(Reg address, Reg value) {
    _jit_new_node_ww(m_state, jit_code_str_i, reg(address), reg(value));
}

void LightningJIT::sti(Address address, Reg value) {
    _jit_new_node_pw(m_state, jit_code_sti_i, address, reg(value));
}

void LightningJIT::stxi(int offset, Reg address, Reg value) {
    _jit_new_node_www(m_state, jit_code_stxi_i, offset, reg(address), reg(value));
}

void LightningJIT::prolog() {
    _jit_prolog(m_state);
}

JIT::Label LightningJIT::arg() {
    m_labels.emplace_back(_jit_arg(m_state));
    return m_labels.size() - 1;
}

void LightningJIT::getarg(Reg target, Label arg) {
    _jit_getarg_i(m_state, reg(target), m_labels[arg]);
}

void LightningJIT::allocai(int stackSizeBytes) {
    m_stackBase = _jit_allocai(m_state, stackSizeBytes);
}

void LightningJIT::frame(int stackSizeBytes) {
    _jit_frame(m_state, stackSizeBytes);
}

void LightningJIT::ret() {
    _jit_ret(m_state);
}

void LightningJIT::retr(Reg r) {
    _jit_retr(m_state, reg(r));
}

void LightningJIT::reti(int value) {
    _jit_reti(m_state, value);
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

int LightningJIT::reg(Reg r) {
    // For function calls from JITted code, we will assume that all allocated registers need to be saved, and so
    // the distinction between caller-save and callee-save registers seems less important. However, more research
    // should be done here when implementing function calls.
    if (r < JIT_R_NUM) {
        return JIT_R(r);
    }
    if (r - JIT_R_NUM < JIT_V_NUM) {
        return JIT_V(r - JIT_R_NUM);
    }
    m_errorReporter->addInternalError(fmt::format("LightningJIT got request for %r{}, but there are only {} machine "
        "registers", r, JIT_R_NUM + JIT_V_NUM));
    return r;
}

} // namespace hadron