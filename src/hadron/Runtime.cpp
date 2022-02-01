#include "hadron/Runtime.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"
#include "internal/FileSystem.hpp"

#include "spdlog/spdlog.h"

#include <array>

namespace hadron {

Runtime::Runtime(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter),
    m_heap(std::make_shared<Heap>()),
    m_threadContext(std::make_unique<ThreadContext>()),
    m_classLibrary(std::make_unique<ClassLibrary>(errorReporter)) {
    LighteningJIT::initJITGlobals();
    m_threadContext->heap = m_heap;
    m_threadContext->symbolTable = std::make_unique<SymbolTable>();
}

Runtime::~Runtime() {}

bool Runtime::compileClassLibrary() {
    auto classLibPath = findSCClassLibrary();
    SPDLOG_INFO("Starting Class Library compilation for files at {}", classLibPath.c_str());
    m_classLibrary->addClassDirectory(classLibPath);
    return m_classLibrary->compileLibrary(m_threadContext.get());
}

bool Runtime::initInterpreter() {
    if (!buildTrampolines()) return false;
    if (!buildThreadContext()) return false;
    return true;
}

bool Runtime::buildTrampolines() {
    LighteningJIT::markThreadForJITCompilation();
    size_t jitBufferSize = 0;
    library::Int8Array jitArray = library::Int8Array::arrayAllocJIT(m_threadContext.get(), Heap::kSmallObjectSize,
            jitBufferSize);
    m_heap->addToRootSet(jitArray.slot());
    LighteningJIT jit;
    jit.begin(jitArray.start(), jitArray.capacity(m_threadContext.get()));
    auto align = jit.enterABI();
    // Loads the (assumed) two arguments to the entry trampoline, ThreadContext* m_threadContext and a uint8_t*
    // machineCode pointer. The threadContext is loaded into the kContextPointerReg, and the code pointer is loaded into
    // Reg 0. As Lightening re-uses the C-calling convention stack register JIT_SP as a general-purpose register, I have
    // taken some care to ensure that GPR(2)/Reg 0 is not the stack pointer on any of the supported architectures.
    jit.loadCArgs2(JIT::kContextPointerReg, JIT::Reg(0));
    // Save the C stack pointer, this pointer is *not* tagged as it does not point into Hadron-allocated heap.
    jit.stxi_w(offsetof(ThreadContext, cStackPointer), JIT::kContextPointerReg, jit.getCStackPointerRegister());
    // Restore the Hadron stack pointer.
    jit.ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, stackPointer));
    // Remove tag from stack pointer.
    jit.andi(JIT::kStackPointerReg, JIT::kStackPointerReg, ~Slot::kTagMask);
    // Jump into the calling code.
    jit.jmpr(JIT::Reg(0));

    m_exitTrampoline = jit.addressToFunctionPointer(jit.address());
    // Restore the C stack pointer.
    jit.ldxi_w(jit.getCStackPointerRegister(), JIT::kContextPointerReg, offsetof(ThreadContext, cStackPointer));
    jit.leaveABI(align);
    jit.ret();
    assert(!jit.hasJITBufferOverflow());
    size_t trampolineSize = 0;
    auto entryAddr = jit.end(&trampolineSize);
    m_entryTrampoline = reinterpret_cast<void (*)(ThreadContext*, const uint8_t*)>(
            jit.addressToFunctionPointer(entryAddr));
    jitArray.resize(m_threadContext.get(), trampolineSize);
    SPDLOG_INFO("Runtime built JIT trampoline at {} bytes out of {} max.", trampolineSize, jitBufferSize);

    return true;
}

bool Runtime::buildThreadContext() {
    m_threadContext->thisProcess = library::Process::alloc(m_threadContext.get()).instance();
    return true;
}

void Runtime::enterMachineCode(const uint8_t* machineCode) {
    // Set machine return address as the exit trampoline into Hadron stack frame.
    *(m_threadContext->framePointer) = Slot::makePointer(m_threadContext->framePointer);
    --(m_threadContext->framePointer);
    *(m_threadContext->framePointer) = Slot::makePointer(m_threadContext->stackPointer);
    --(m_threadContext->framePointer);
    *(m_threadContext->framePointer) = Slot::makePointer(reinterpret_cast<uint8_t*>(m_exitTrampoline));
    --(m_threadContext->framePointer);

    // Initialize return value.
    *(m_threadContext->framePointer) = Slot::makeNil();
    // No arguments means stack pointer == frame pointer.
    m_threadContext->stackPointer = m_threadContext->framePointer;

    // Set up exit state.
    m_threadContext->exitMachineCode = reinterpret_cast<uint8_t*>(m_exitTrampoline);
    m_threadContext->machineCodeStatus = 0;

    // Hit the trampoline.
    SPDLOG_INFO("Machine code entry.");
    m_entryTrampoline(m_threadContext.get(), machineCode);
    SPDLOG_INFO("Machine code exit.");
}

} // namespace hadron