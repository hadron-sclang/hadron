#include "hadron/LighteningJIT.hpp"

#include "hadron/ErrorReporter.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc99-extensions"
extern "C" {
#include "lightening.h"
}
#pragma GCC diagnostic pop

namespace {
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
}

namespace hadron {


LighteningJIT::LighteningJIT(std::shared_ptr<ErrorReporter> errorReporter):
    JIT(errorReporter),
    m_stackBase(0) {
    m_state = jit_new_state(malloc, free);
}

LighteningJIT::~LighteningJIT() {
    jit_destroy_state(m_state);
}

// static
bool LighteningJIT::markThreadForJITCompilation() {
    pthread_jit_write_protect_np(false);
    if (!init_jit()) {
        SPDLOG_ERROR("Failed to initialize thread-specific Lightening JIT data.");
        return false;
    }
    return true;
}

// static
void LighteningJIT::markThreadForJITExecution() {
    pthread_jit_write_protect_np(true);
}

void LighteningJIT::begin(uint8_t* buffer, size_t size) {
    jit_begin(m_state, buffer, size);
}

bool LighteningJIT::hasJITBufferOverflow() {
    return jit_has_overflow(m_state);
}

void LighteningJIT::reset() {
    jit_reset(m_state);
}

void* LighteningJIT::end(size_t* sizeOut) {
    return jit_end(m_state, sizeOut);
}

size_t LighteningJIT::enterABI() {
    auto align = jit_enter_jit_abi(m_state, kCalleeSaveRegisters, 0, 0);
    return align;
}

void LighteningJIT::loadCArgs2(Reg arg1, Reg arg2) {
    jit_operand_t a;
    a.abi = JIT_OPERAND_ABI_POINTER;
    a.kind = JIT_OPERAND_KIND_GPR;
    a.loc.gpr.gpr = reg(arg1);
    a.loc.gpr.addend = 0;

    jit_operand_t b;
    b.abi = JIT_OPERAND_ABI_POINTER;
    b.kind = JIT_OPERAND_KIND_GPR;
    b.loc.gpr.gpr = reg(arg2);
    b.loc.gpr.addend = 0;

    jit_load_args_2(m_state, a, b);
}

JIT::Reg LighteningJIT::getCStackPointerRegister() const {
#   if defined(__i386__)
    Reg r = 4;   // JIT_SP = JIT_GPR(4)
#   elif defined(__x86_64__)
    Reg r = 4;   // JIT_SP = JIT_GPR(4)
#   elif defined(__arm__)
    Reg r = 13;  // JIT_SP = JIT_GPR(13)
#   elif defined(__aarch64__)
    Reg r = 31;  // JIT_SP = JIT_GPR(31)
#   else
#   error "Undefined chipset"
#   endif
    r = r - 2;  // Adjust register number for the two reserved registers.

#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wc99-extensions"
    assert(jit_same_gprs(reg(r), JIT_SP));
#   pragma GCC diagnostic pop

    return r;
}

void LighteningJIT::leaveABI(size_t stackSize) {
    return jit_leave_jit_abi(m_state, kCalleeSaveRegisters, 0, stackSize);
}

LighteningJIT::FunctionPointer LighteningJIT::addressToFunctionPointer(Address a) {
    return jit_address_to_function_pointer(m_addresses[a]);
}

int LighteningJIT::getRegisterCount() const {
    // Two registers always reserved for context pointer and stack pointer.
#   if defined(__i386__)
    return 8 - 2;
#   elif defined(__x86_64__)
    return 16 - 2;
#   elif defined(__arm__)
    return 16 - 2;
#   elif defined(__aarch64__)
    return 32 - 2;
#   else
#   error "Undefined chipset"
#   endif
}

int LighteningJIT::getFloatRegisterCount() const {
#   if defined(__i386__)
    return 8;
#   elif defined(__x86_64__)
    return 16;
#   elif defined(__arm__)
    return 32;
#   elif defined(__aarch64__)
    return 32;
#   else
#   error "Undefined chipset"
#   endif
}

void LighteningJIT::addr(Reg target, Reg a, Reg b) {
    jit_addr(m_state, reg(target), reg(a), reg(b));
}

void LighteningJIT::addi(Reg target, Reg a, int b) {
    jit_addi(m_state, reg(target), reg(a), b);
}

void LighteningJIT::movr(Reg target, Reg value) {
    if (target != value) {
        jit_movr(m_state, reg(target), reg(value));
    }
}

void LighteningJIT::movi(Reg target, int value) {
    jit_movi(m_state, reg(target), value);
}

JIT::Label LighteningJIT::bgei(Reg a, int b) {
    m_labels.emplace_back(jit_bgei(m_state, reg(a), b));
    return m_labels.size() - 1;
}

JIT::Label LighteningJIT::jmp() {
    m_labels.emplace_back(jit_jmp(m_state));
    return m_labels.size() - 1;
}

void LighteningJIT::jmpr(Reg r) {
    jit_jmpr(m_state, reg(r));
}

void LighteningJIT::ldxi_w(Reg target, Reg address, int offset) {
    if (sizeof(void*) == 8) {
        ldxi_l(target, address, offset);
    } else {
        ldxi_i(target, address, offset);
    }
}

void LighteningJIT::ldxi_i(Reg target, Reg address, int offset) {
    jit_ldxi_i(m_state, reg(target), reg(address), offset);
}

void LighteningJIT::ldxi_l(Reg target, Reg address, int offset) {
    jit_ldxi_l(m_state, reg(target), reg(address), offset);
}

void LighteningJIT::str_i(Reg address, Reg value) {
    jit_str_i(m_state, reg(address), reg(value));
}

void LighteningJIT::stxi_w(int offset, Reg address, Reg value) {
    if (sizeof(void*) == 8) {
        stxi_l(offset, address, value);
    } else {
        stxi_i(offset, address, value);
    }
}

void LighteningJIT::stxi_i(int offset, Reg address, Reg value) {
    jit_stxi_i(m_state, offset, reg(address), reg(value));
}

void LighteningJIT::stxi_l(int offset, Reg address, Reg value) {
    jit_stxi_l(m_state, offset, reg(address), reg(value));
}

void LighteningJIT::ret() {
    jit_ret(m_state);
}

void LighteningJIT::retr(Reg r) {
    jit_retr(m_state, reg(r));
}

void LighteningJIT::reti(int value) {
    jit_reti(m_state, value);
}

JIT::Label LighteningJIT::label() {
    m_labels.emplace_back(jit_emit_addr(m_state));
    return m_labels.size() - 1;
}

JIT::Address LighteningJIT::address() {
    JIT::Address addressIndex = m_addresses.size();
    m_addresses.emplace_back(jit_address(m_state));
    return addressIndex;
}

void LighteningJIT::patchHere(Label label) {
    jit_patch_here(m_state, m_labels[label]);
}

void LighteningJIT::patchThere(Label target, Address location) {
    jit_patch_there(m_state, m_labels[target], m_addresses[location]);
}

jit_gpr_t LighteningJIT::reg(Reg r) const {
    assert(r < getRegisterCount());
    // Account for the two reserved registers.
    r = r + 2;
    jit_gpr gpr;
    gpr.regno = r;
    return gpr;
}

} // namespace hadron