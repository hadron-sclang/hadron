#include "hadron/Interpreter.hpp"

#include "hadron/Compiler.hpp"
#include "hadron/JITMemoryArena.hpp"
#include "hadron/LighteningJIT.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Interpreter::Interpreter(): m_compiler(std::make_unique<Compiler>()) { }

Interpreter::~Interpreter() {
    stop();
}

bool Interpreter::start() {
    if (!m_compiler->start()) {
        return false;
    }

    // Compile the entry and exit trampolines. This matches the Guile entry/exit trampolines pretty closely.
    LighteningJIT::markThreadForJITCompilation();
    // Complete guess as to a reasonable size.
    JITMemoryArena::MCodePtr m_trampolines = compiler->jitMemoryArena()->alloc(4096);
    LighteningJIT jit(/* need an error reporter */);
    jit.begin(trampolines.get(), 4096);
    m_entryTrampoline = jit_address_to_function_pointer(jit.address());
    auto align = jit.enterABI();
    Also loads the (assumed) two arguments to the entry trampoline, ThreadContext* context and a uint8_t*
    // machineCode pointer. The threadContext is loaded into the kContextPointerReg, and the code pointer is loaded
    // into Reg 0. As Lightening re-uses the C-calling convention stack register JIT_SP as a general-purpose register,
    // I have taken some care to ensure that GPR(2)/Reg 0 is not the stack pointer on any of the supported
    // architectures.
    jit.loadCArgs2(JIT::kContextPointerReg, JIT::Reg(0));
    // Save the C stack pointer.
    jit.stxi_w(offsetof(ThreadContext, cStackPointer), JIT::kContextPointerReg, jit.getCStackPointerRegister());
    // Restore the Hadron stack pointer
    jit.ldxi_w(JIT::kStackPointerReg, offsetof(ThreadContext, stackPointer), JIT::kContextPointerReg);
    // Jump into the calling code.
    jit.jmpr(0);

    m_exitTrampoline = jit_address_to_function_pointer(jit.address());
    jit.leaveABI(align);
    jit.ret();
}

void Interpreter::stop() {

}

std::unique_ptr<Function> Interpreter::compile(std::string_view code) {

}

std::unique_ptr<Function> Interpreter::compileFile() {

}

Slot Interpreter::run(Function* func) {

}

void Interpreter::enterMachineCode(ThreadContext* context, const uint8_t* machineCode) {
    // Set machine return address as the exit trampoline into Hadron stack frame.
    *(context->framePointer) = Slot(Type::kFramePointer, context->framePointer);
    --(context->framePointer);
    *(context->framePointer) = Slot(Type::kFramePointer, context->stackPointer);
    --(context->framePointer);
    *(context->framePointer) = Slot(static_cast<uint8_t*>(m_exitTrampoline));
    --(context->framePointer);

    // Initialize return value.
    *(context->framePointer) = Slot();
    // No arguments means stack pointer == frame pointer.
    context->stackPointer = context->framePointer;

    // Set up exit state.
    context->exitMachineCode = static_cast<uint8_t*>(m_exitTrampoline);
    context->machineCodeStatus = 0;

    // Hit the trampoline.
    SPDLOG_INFO("Machine code entry.");
    m_entryTrampoline(context, machineCode);
    SPDLOG_INFO("Machine code exit.")
}

}  // namespace hadron