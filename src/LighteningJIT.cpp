#include "hadron/LighteningJIT.hpp"

#include "hadron/ErrorReporter.hpp"

#include "fmt/format.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc99-extensions"
extern "C" {
#include "lightening/lightening.h"
}
#pragma GCC diagnostic pop

namespace hadron {


LighteningJIT::LighteningJIT(std::shared_ptr<ErrorReporter> errorReporter):
    JIT(errorReporter),
    m_stackBase(0) {
    m_state = jit_new_state(malloc, free);
}

LighteningJIT::~LighteningJIT() {
    jit_destroy_state(m_state);
}

bool LighteningJIT::emit() {
    m_jit = reinterpret_cast<Value>(jit_emit(m_state));
    return (m_jit != nullptr);
}

bool LighteningJIT::evaluate(Slot* value) const {
    return m_jit(value) != 0;
}

int LighteningJIT::getRegisterCount() const {
    return JIT_R_NUM + JIT_V_NUM;
}

int LighteningJIT::getFloatRegisterCount() const {
    return JIT_F_NUM;
}

void LighteningJIT::addr(Reg target, Reg a, Reg b) {
    _jit_new_node_www(m_state, jit_code_addr, reg(target), reg(a), reg(b));
}

void LighteningJIT::addi(Reg target, Reg a, int b) {
    _jit_new_node_www(m_state, jit_code_addi, reg(target), reg(a), b);
}

void LighteningJIT::movr(Reg target, Reg value) {
    if (target != value) {
        _jit_new_node_ww(m_state, jit_code_movr, reg(target), reg(value));
    }
}

void LighteningJIT::movi(Reg target, int value) {
    _jit_new_node_ww(m_state, jit_code_movi, reg(target), value);
}

JIT::Label LighteningJIT::bgei(Reg a, int b) {
    m_labels.emplace_back(_jit_new_node_pww(m_state, jit_code_bgei, nullptr, reg(a), b));
    return m_labels.size() - 1;
}

JIT::Label LighteningJIT::jmpi() {
    m_labels.emplace_back(_jit_new_node_p(m_state, jit_code_jmpi, nullptr));
    return m_labels.size() - 1;
}

void LighteningJIT::ldxi_w(Reg target, Reg address, int offset) {
    if (sizeof(void*) == 8) {
        ldxi_l(target, address, offset);
    } else {
        ldxi_i(target, address, offset);
    }
}

void LighteningJIT::ldxi_i(Reg target, Reg address, int offset) {
    _jit_new_node_www(m_state, jit_code_ldxi_i, reg(target), reg(address), offset);
}

void LighteningJIT::ldxi_l(Reg target, Reg address, int offset) {
    _jit_new_node_www(m_state, jit_code_ldxi_l, reg(target), reg(address), offset);
}

void LighteningJIT::str_i(Reg address, Reg value) {
    _jit_new_node_ww(m_state, jit_code_str_i, reg(address), reg(value));
}

void LighteningJIT::sti_i(Address address, Reg value) {
    _jit_new_node_pw(m_state, jit_code_sti_i, address, reg(value));
}

void LighteningJIT::stxi_w(int offset, Reg address, Reg value) {
    if (sizeof(void*) == 8) {
        stxi_l(offset, address, value);
    } else {
        stxi_i(offset, address, value);
    }
}

void LighteningJIT::stxi_i(int offset, Reg address, Reg value) {
    _jit_new_node_www(m_state, jit_code_stxi_i, offset, reg(address), reg(value));
}

void LighteningJIT::stxi_l(int offset, Reg address, Reg value) {
    _jit_new_node_www(m_state, jit_code_stxi_l, offset, reg(address), reg(value));
}

void LighteningJIT::prolog() {
    _jit_prolog(m_state);
}

JIT::Label LighteningJIT::arg() {
    m_labels.emplace_back(_jit_arg(m_state));
    return m_labels.size() - 1;
}

void LighteningJIT::getarg_w(Reg target, Label arg) {
    if (sizeof(void*) == 8) {
        getarg_l(target, arg);
    } else {
        getarg_i(target, arg);
    }
}

void LighteningJIT::getarg_i(Reg target, Label arg) {
    _jit_getarg_i(m_state, reg(target), m_labels[arg]);
}

void LighteningJIT::getarg_l(Reg target, Label arg) {
    _jit_getarg_l(m_state, reg(target), m_labels[arg]);
}

void LighteningJIT::allocai(int stackSizeBytes) {
    // Lightening JIT hangs on emit() if given a zero value for allocai or frame, so keep to some small nonzero min
    // value.
    auto stackSize = std::max(16, stackSizeBytes);
    m_stackBase = _jit_allocai(m_state, stackSize);
}

void LighteningJIT::frame(int stackSizeBytes) {
    auto stackSize = std::max(16, stackSizeBytes);
    _jit_frame(m_state, stackSize);
}

void LighteningJIT::ret() {
    _jit_ret(m_state);
}

void LighteningJIT::retr(Reg r) {
    _jit_retr(m_state, reg(r));
}

void LighteningJIT::reti(int value) {
    _jit_reti(m_state, value);
}

void LighteningJIT::epilog() {
    _jit_epilog(m_state);
}

JIT::Label LighteningJIT::label() {
    m_labels.emplace_back(_jit_label(m_state));
    return m_labels.size() - 1;
}

void LighteningJIT::patchAt(Label target, Label location) {
    _jit_patch_at(m_state, m_labels[target], m_labels[location]);
}

void LighteningJIT::patch(Label label) {
    _jit_patch(m_state, m_labels[label]);
}

// static
void LighteningJIT::initJITGlobals() {
    init_jit(nullptr);
}

int LighteningJIT::reg(Reg r) {
    if (r == kFramePointerReg) {
        return JIT_FP;
    }
    // For function calls from JITted code, we will assume that all allocated registers need to be saved, and so
    // the distinction between caller-save and callee-save registers seems less important. However, more research
    // should be done here when implementing function calls.
    if (r < JIT_R_NUM) {
        return JIT_R(r);
    }
    if (r - JIT_R_NUM < JIT_V_NUM) {
        return JIT_V(r - JIT_R_NUM);
    }
    m_errorReporter->addInternalError(fmt::format("LighteningJIT got request for %r{}, but there are only {} machine "
        "registers", r, JIT_R_NUM + JIT_V_NUM));
    return r;
}


} // namespace hadron