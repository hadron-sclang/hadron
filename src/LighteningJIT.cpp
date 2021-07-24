#include "hadron/LighteningJIT.hpp"

#include "hadron/ErrorReporter.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

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


int LighteningJIT::getRegisterCount() const {
#if defined(__i386__)
    return 8;
#elif defined(__x86_64__)
    return 16;
#elif defined(__arm__)
    return 16;
#elif defined(__aarch64__)
    return 32;
#else
#error "Undefined chipset"
#endif
}

int LighteningJIT::getFloatRegisterCount() const {
#if defined(__i386__)
    return 8;
#elif defined(__x86_64__)
    return 16;
#elif defined(__arm__)
    return 32;
#elif defined(__aarch64__)
    return 32;
#else
#error "Undefined chipset"
#endif
}

// KEY realization is that the hadron stack can have its arguments prepared for on the C side, when calling
// from C. It's just memory and can be manipulated by both C and machine code. So, to call from C we assume
// the stack has already been prepared as follows. The address of the instruction after the jmp becomes the
// return address we save on the hadron stack, and that address also gets saved in thread context as the
// exitMachineCode pointer. There's a machineCodeStatus marker that could indicate normal completion, an
// exception/error code, an intrinsic callout, or a call to a regular c function. Callouts could then
// re-enter via the trampoline.

// Function - pushArgs (copies args into the hadron stack, C++ code to call at runtime)
// Then the trampoline just enters hadron stack frame, saves C stack pointer, sets up threadContext and frame pointer
// then jumps.

size_t LighteningJIT::enterABI() {
    auto align = jit_enter_jit_abi(m_state, kCalleeSaveRegisters, 0, 0);



    // On function entry, JITted code expects a pointer to ThreadContext in GPR0, and the arguments to the function
    // in order on the stack starting with argument 0 at SP + sizeof(void*) for the calling address. Each Slot is
    // 16 bytes, so stack looks like: (higher values of stack address at top)

    // Keyword/Arg Slot pairs
    // Number of Keyword/Arg pairs: word size
    // Number of Ordered Args: word size
    // Hash of Target type: 8 bytes (ignored after dispatch)
    // Hash of Selector name: 8 bytes (ignored after dispatch)
    // Caller Return Address ======= SP
    // Return Value Slot             SP - AddressSize - will be at the top of the stack after return
    // ARG 0                         SP - AddressSize
    // ARG 1                         SP - AddressSize - 16
    // ....
    // ARG N-1                       SP - AddressSize - (16 * (N -1))

    // <--- start of scratch space for callees
    // Virtual Reg Spill 0
    // Virtual Reg Spill 1
    // ...

    // On the caller side, there will be a count and list of ordered args, then a count and list of keyword/arg pairs.
    // Calling from within Hadron ABI, there's a part where we do dispatch using target and selector hashes to find
    // this function object, then we have the same entry conditions as here, which is that there are the two lists
    // of arguments and their counts available somewhere.

    jit_operand_t args[5];
    args[0].abi = JIT_OPERAND_ABI_POINTER;  // ThreadContext* context
    args[1].abi = JIT_OPERAND_ABI_INT32;    // int numOrderedArgs
    args[2].abi = JIT_OPERAND_ABI_POINTER;  // Slot* orderedArgs
    args[3].abi = JIT_OPERAND_ABI_INT32;    // int numKeywordArgs
    args[4].abi = JIT_OPERAND_ABI_POINTER;  // Slot* keywordArgs
    jit_locate_args(m_state, 5, args);

    // We need 4 registers to set up the stack, but have to pick registers that aren't going to contain
    // arguments, and we can't modify the stack pointer while extracting arguments, because some arguments
    // may be on the stack.
#if defined(__i386__)
    // 32-bit ABI passes no arguments in registers
    jit_gpr_t stackPointer = JIT_GPR0;
    jit_gpr_t argumentPointer = JIT_GPR1;
    jit_gpr_t loopCounter = JIT_GPR2;
    jit_gpr_t value = JIT_GPR3;
#elif defined(__x86_64__)
    jit_gpr_t stackPointer = JIT_GPR0;
    jit_gpr_t argumentPointer = JIT_GPR3;
    jit_gpr_t loopCounter = JIT_GPR5;
    jit_gpr_t value = JIT_GPR3;
#elif defined(__arm__)

#elif defined(__aarch64__)
    return 32;
#else
#error "Undefined chipset"
#endif


    // First thing is to jit a loop to push the keyword/arg pairs onto the stack. The argument locations
    // returned by lightening are stable for a given CPU and OS combination. If arguments are passed in registers
    // they are always passed in *caller-save* or volatile registers, meaning that if we load them into
    // *callee-save* registers we can be confident that we have not cloberred other arguments. We could use the
    // jit_load_args() function, but with 5 arguments and only 3 guaranteed registers it's clearer to
    // manually move stuff into place here.
    if (args[3].kind == JIT_OPERAND_KIND_GPR) {
        jit_movr(m_state, JIT_R0, args[3].loc.gpr.gpr);
    } else if (args[3].kind == JIT_OPERAND_KIND_MEM) {
        jit_ldxi(m_state, JIT_R0, args[3].loc.mem.base, args[3].loc.mem.offset);
    } else {
        SPDLOG_CRITICAL("Failed to locate arg!")
        return 0;
    }
    // Load the pointer into R1 (make this a function!)
    if (args[4].kind == JIT_OPERAND_KIND_GPR) {
        jit_movr(m_state, JIT_R1, args[4].loc.gpr.gpr);
    } else if (args[4].kind == JIT_OPERAND_KIND_MEM) {
        jit_ldxi(m_state, JIT_R1, args[4].loc.mem.base, args[4].loc.mem.offset);
    } else {
        SPDLOG_CRITICAL("Failed to locate arg!")
        return 0;
    }

    // Branch if zero argcount, otherwise load stuff into r2, increment pointer, push onto stack
    auto skipLoop = jit_beqi(m_state, JIT_V0, 0);
    auto loopEntry = jit_emit_addr(m_state);
    jit_ldr(m_state, JIT_V2, JIT_V1);
    jit_str(m_state, JIT_SP, JIT_V2);
    jit_ldxi(m_state, JIT_V2, JIT_V1, 8);  // TODO: 64-bit assumption here
    jit_stxi(m_state, 8, JIT_SP, JIT_V2);
    jit_addi(m_state, JIT_V1, JIT_V1, 16);
    jit_subi(m_state, JIT_SP, JIT_SP, 16);
    jit_subi(m_state, JIT_V0, JIT_V0, 1);
    jit_bgti(m_state, loopEntry, JIT_V0, 0);
    jit_patch_here(m_state, skipLoop);

    // Push Keyword/Arg pairs onto stack - question about fetching further args at this point - you've blown
    // up the stack pointer! So we need another register to use as a stack pointer. This is probably machine
    // dependent and possibly os-dependent too.






    return align;
}

void LighteningJIT::leaveABI(size_t stackSize) {
    return jit_leave_jit_abi(m_state, kCalleeSaveRegisters, 0, stackSize);
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
    // JIT_GPR(4) - this is the stack register - 

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