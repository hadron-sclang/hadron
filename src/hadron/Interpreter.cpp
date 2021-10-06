#include "hadron/Interpreter.hpp"

#include "hadron/BlockBuilder.hpp"
#include "hadron/BlockSerializer.hpp"
#include "hadron/Emitter.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LifetimeAnalyzer.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
#include "hadron/RegisterAllocator.hpp"
#include "hadron/Resolver.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/ThreadContext.hpp"
#include "hadron/VirtualJIT.hpp"
#include "internal/FileSystem.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Interpreter::Interpreter():
    m_errorReporter(std::make_shared<ErrorReporter>()),
    m_jitMemoryArena(std::make_unique<JITMemoryArena>()) {}

Interpreter::~Interpreter() {
    teardown();
}

bool Interpreter::setup() {
    // Creating the arena requires allocating memory in that arena, and therefore writing to the arena, so we need to
    // ask for write permissions to the arena in order to create it.
    LighteningJIT::markThreadForJITCompilation();

    if (!m_jitMemoryArena->createArena()) {
        SPDLOG_ERROR("Failed to create JIT memory arena.");
        return false;
    }

    // Compile the entry and exit trampolines. This matches the Guile entry/exit trampolines pretty closely.
    LighteningJIT::markThreadForJITCompilation();
    // Complete guess as to a reasonable size.
    m_trampolines = m_jitMemoryArena->alloc(256);
    LighteningJIT jit(m_errorReporter);
    jit.begin(m_trampolines.get(), 256);
    auto align = jit.enterABI();
    // Loads the (assumed) two arguments to the entry trampoline, ThreadContext* context and a uint8_t* machineCode
    // pointer. The threadContext is loaded into the kContextPointerReg, and the code pointer is loaded into Reg 0. As
    // Lightening re-uses the C-calling convention stack register JIT_SP as a general-purpose register, I have taken
    // some care to ensure that GPR(2)/Reg 0 is not the stack pointer on any of the supported architectures.
    jit.loadCArgs2(JIT::kContextPointerReg, JIT::Reg(0));
    // Save the C stack pointer, this pointer is *not* tagged as it does not point into Hadron-allocated heap.
    jit.stxi_w(offsetof(ThreadContext, cStackPointer), JIT::kContextPointerReg, jit.getCStackPointerRegister());
    // Restore the Hadron stack pointer
    jit.ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, stackPointer));
    // Remove tag from pointer.
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
    SPDLOG_INFO("JIT trampoline at {} bytes.", trampolineSize);

    return true;
}

void Interpreter::teardown() {
    // Free jit memory before destroying memory arena, or it will be reported as a leak.
    m_trampolines.reset();
    if (m_jitMemoryArena) {
        m_jitMemoryArena->destroyArena();
        m_jitMemoryArena.reset();
    }
}

std::unique_ptr<Function> Interpreter::compile(std::string_view code) {
    LighteningJIT::markThreadForJITCompilation();
    m_errorReporter->setCode(code.data());

    Lexer lexer(code, m_errorReporter);
    if (!lexer.lex() || !m_errorReporter->ok()) {
        SPDLOG_DEBUG("Lexing failed");
        return nullptr;
    }

    Parser parser(&lexer, m_errorReporter);
    if (!parser.parse() || !m_errorReporter->ok()) {
        SPDLOG_DEBUG("Parsing failed");
        return nullptr;
    }

    BlockBuilder blockBuilder(&lexer, m_errorReporter);
    auto frame = blockBuilder.buildFrame(reinterpret_cast<const hadron::parse::BlockNode*>(parser.root()));

    BlockSerializer blockSerializer;
    auto linearBlock = blockSerializer.serialize(std::move(frame), LighteningJIT::physicalRegisterCount());

    LifetimeAnalyzer lifetimeAnalyzer;
    lifetimeAnalyzer.buildLifetimes(linearBlock.get());

    RegisterAllocator registerAllocator;
    registerAllocator.allocateRegisters(linearBlock.get());

    Resolver resolver;
    resolver.resolve(linearBlock.get());

    // Build function object.
    auto function = std::make_unique<Function>();
    // TODO: setup args inside function.

    // Estimate jit buffer size to be the byte size of the generated HIR bytecode * some constant size. If this is too
    // large by far it's potentially wasteful, we can call xallocx() to downsize the buffer, an extended jemalloc
    // function that doesn't copy buffers when reallocating. It is unknown what kind of pressure this will cause on the
    // memory allocator but I assume the allocator performs better on allocations when the size doesn't change after
    // allocation. Note that a a buffer copy, like realloc() may perform, will break the JIT code inside, as it is not
    // relocateable. If the machine code ends up being larger then the allocated buffer we double the projected size and
    // regenerate the JIT, repeating until the generated code fits. Therefore accurate estimates of generated bytecode
    // size are useful in preventing excessive memory waste or re-rendering.
    size_t machineCodeSize = (linearBlock->instructions.size() * 16) + 128;
    auto machineCode = m_jitMemoryArena->alloc(machineCodeSize);

    while (true) {
        if (!machineCode) {
            SPDLOG_CRITICAL("Failed to allocate JIT memory!");
            return nullptr;
        }

        LighteningJIT jit(m_errorReporter);
        jit.begin(machineCode.get(), machineCodeSize);

        hadron::Emitter emitter;
        emitter.emit(linearBlock.get(), &jit);

        if (!jit.hasJITBufferOverflow()) {
            size_t jitSize = 0;
            const uint8_t* codeStart = jit.getAddress(jit.end(&jitSize));
            SPDLOG_INFO("JIT completed, buffer size {} bytes, jit size {} bytes.", machineCodeSize, jitSize);
            function->machineCode = codeStart;
            function->machineCodeOwned = std::move(machineCode);
            break;
        }
        // JIT buffer overflow, double the size of the allocation and try to regenerate the JIT.
        SPDLOG_INFO("JIT buffer of {} size too small, doubling.", machineCodeSize);
        machineCodeSize = 2 * machineCodeSize;
        machineCode = m_jitMemoryArena->alloc(machineCodeSize);
    }

    return function;
}

std::unique_ptr<Function> Interpreter::compileFile(std::string path) {
    SourceFile file(path);
    if (!file.read(m_errorReporter)) {
        return nullptr;
    }

    return compile(std::string_view(file.codeView()));
}

bool Interpreter::compileClass(std::string path) {
    SourceFile file(path);
    if (!file.read(m_errorReporter)) {
        return false;
    }


}

Slot Interpreter::run(Function* func) {
    ThreadContext threadContext;
    if (!threadContext.allocateStack()) {
        SPDLOG_ERROR("Failed to allocate Hadron stack.");
        return Slot();
    }

    LighteningJIT::markThreadForJITExecution();

    // Trampoline into JIT code.
    enterMachineCode(&threadContext, func->machineCode);

    // Any memory allocation in the JIT arena requires the thread be marked for compilation, which allows writing to
    // the JIT memory regions. TODO: we need a better threading model to isolate the execution threads from the
    // rest of the system.
    LighteningJIT::markThreadForJITCompilation();

    // Extract result from stack.
    Slot result = *(threadContext.framePointer);
    return result;
}

void Interpreter::enterMachineCode(ThreadContext* context, const uint8_t* machineCode) {
    // Set machine return address as the exit trampoline into Hadron stack frame.
    *(context->framePointer) = context->framePointer;
    --(context->framePointer);
    *(context->framePointer) = context->stackPointer;
    --(context->framePointer);
    *(context->framePointer) = reinterpret_cast<uint8_t*>(m_exitTrampoline);
    --(context->framePointer);

    // Initialize return value.
    *(context->framePointer) = Slot();
    // No arguments means stack pointer == frame pointer.
    context->stackPointer = context->framePointer;

    // Set up exit state.
    context->exitMachineCode = reinterpret_cast<uint8_t*>(m_exitTrampoline);
    context->machineCodeStatus = 0;

    // Hit the trampoline.
    SPDLOG_INFO("Machine code entry.");
    m_entryTrampoline(context, machineCode);
    SPDLOG_INFO("Machine code exit.");
}

}  // namespace hadron