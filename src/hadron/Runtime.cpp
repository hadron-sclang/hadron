#include "hadron/Runtime.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/Heap.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/library/Thread.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"
#include "internal/FileSystem.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

Runtime::Runtime():
    m_heap(std::make_shared<Heap>()),
    m_threadContext(std::make_unique<ThreadContext>()) {
    LighteningJIT::initJITGlobals();
    m_threadContext->heap = m_heap;
    m_threadContext->symbolTable = std::make_unique<SymbolTable>();
    m_threadContext->classLibrary = std::make_unique<ClassLibrary>();
}

Runtime::~Runtime() {}

bool Runtime::initInterpreter() {
    if (!buildThreadContext()) return false;
    if (!buildTrampolines()) return false;
    m_threadContext->classLibrary->bootstrapLibrary(m_threadContext.get());
    m_interpreter = library::Interpreter::alloc(m_threadContext.get());
    m_interpreter.initToNil();

    return true;
}

void Runtime::addDefaultPaths() {
    addClassDirectory(findSCClassLibrary());
    addClassDirectory(findHLangClassLibrary());
}

void Runtime::addClassDirectory(const std::string& path) {
    m_libraryPaths.emplace(fs::absolute(path));
}

bool Runtime::scanClassFiles() {
    for (const auto& classLibPath : m_libraryPaths) {
        for (auto& entry : fs::recursive_directory_iterator(classLibPath)) {
            const auto& path = fs::absolute(entry.path());
            if (!fs::is_regular_file(path) || path.extension() != ".sc")
                continue;

            auto sourceFile = std::make_unique<SourceFile>(path.string());
            if (!sourceFile->read()) { return false; }

            auto filename = library::Symbol::fromView(m_threadContext.get(), path.string());
            if (!m_threadContext->classLibrary->scanString(m_threadContext.get(), sourceFile->codeView(), filename)) {
                return false;
            }
        }
    }

    return true;
}

bool Runtime::scanClassString(std::string_view input, std::string_view filename) {
    return m_threadContext->classLibrary->scanString(m_threadContext.get(), input,
            library::Symbol::fromView(m_threadContext.get(), filename));
}

bool Runtime::finalizeClassLibrary() {
    return m_threadContext->classLibrary->finalizeLibrary(m_threadContext.get());
}

Slot Runtime::interpret(std::string_view code) {
    LighteningJIT::markThreadForJITCompilation();

    // TODO: refactor to avoid this needless copy
    auto codeString = library::String::fromView(m_threadContext.get(), code);
    auto function = m_interpreter.compile(m_threadContext.get(), codeString);

    if (!function) { return Slot::makeNil(); }

    // Convention is caller pushes, callee pops.
    auto callerFrame = library::Frame::alloc(m_threadContext.get());
    callerFrame.initToNil();
    callerFrame.setIp(reinterpret_cast<int8_t*>(m_exitTrampoline));

    auto calleeFrame = library::Frame::alloc(m_threadContext.get(),
            std::max(function.def().prototypeFrame().size() - 1, 0));
    calleeFrame.initToNil();
    calleeFrame.setCaller(callerFrame);
    calleeFrame.setContext(calleeFrame);
    calleeFrame.setHomeContext(calleeFrame);
    calleeFrame.setArg0(m_interpreter.slot());
    calleeFrame.copyPrototypeAfterThis(function.def().prototypeFrame());

    m_threadContext->framePointer = calleeFrame.instance();

    auto spareFrame = library::Frame::alloc(m_threadContext.get(), 16);
    spareFrame.initToNil();
    m_threadContext->stackPointer = spareFrame.instance();

    LighteningJIT::markThreadForJITExecution();

    const int8_t* machineCode = function.def().code().start();
    while (true) {
        m_entryTrampoline(m_threadContext.get(), machineCode);
        // If this was a normal completion of the Hadron stack the frame pointer will be pointing at the callerFrame.
        if (m_threadContext->framePointer == callerFrame.instance()) {
            // Extract return value from callee frame pointer, which is our stack pointer.
            return m_threadContext->stackPointer->arg0;
        }

        // This is an interrupt call
        switch (m_threadContext->interruptCode) {
        case ThreadContext::InterruptCode::kDispatch:
            SPDLOG_CRITICAL("Dispatch selector: {}, target: {}",
                    m_threadContext->symbolTable->lookup(m_threadContext->stackPointer->method.getSymbolHash()),
                    slotToString(m_threadContext->stackPointer->arg0));
            break;

        case ThreadContext::InterruptCode::kFatalError:
            SPDLOG_CRITICAL("Fatal Error");
            break;

        case ThreadContext::InterruptCode::kNewObject:
            SPDLOG_CRITICAL("New Object");
            break;

        case ThreadContext::InterruptCode::kPrimitive:
            SPDLOG_CRITICAL("Primitive");
            break;
        }

        break;
    }

    SPDLOG_CRITICAL("frame pointer: {}, calleeFrame: {}, callerFrame: {}",
            reinterpret_cast<void*>(m_threadContext->framePointer),
            reinterpret_cast<void*>(calleeFrame.instance()),
            reinterpret_cast<void*>(callerFrame.instance()));

    return Slot::makeNil();
}

std::string Runtime::slotToString(Slot s) {
    switch(s.getType()) {
    case TypeFlags::kIntegerFlag:
        return fmt::format("{}", s.getInt32());

    case TypeFlags::kFloatFlag: {
        auto doubleStr = fmt::format("{:.14g}", s.getFloat());
        if (doubleStr.find_first_not_of("-0123456789") == std::string::npos) {
            doubleStr += ".0";
        }
        return doubleStr;
    }

    case TypeFlags::kObjectFlag: {
        auto className = library::Symbol(m_threadContext.get(), Slot::makeSymbol(s.getPointer()->_className));
        return fmt::format("a {}", className.view(m_threadContext.get()));
    }

    case TypeFlags::kNilFlag:
        return "nil";
    default:
        return "";
    }
}

bool Runtime::buildThreadContext() {
    m_threadContext->symbolTable->preloadSymbols(m_threadContext.get());
    m_threadContext->thisProcess = library::Process::alloc(m_threadContext.get()).instance();
    m_threadContext->thisThread = library::Thread::alloc(m_threadContext.get()).instance();
    return true;
}

bool Runtime::buildTrampolines() {
    LighteningJIT::markThreadForJITCompilation();
    size_t jitBufferSize = 0;
    library::Int8Array jitArray = library::Int8Array::arrayAllocJIT(m_threadContext.get(), Heap::kSmallObjectSize,
            jitBufferSize);
    LighteningJIT jit;
    jit.begin(jitArray.start(), jitArray.capacity(m_threadContext.get()));
    auto align = jit.enterABI();
    // Loads the (assumed) two arguments to the entry trampoline, ThreadContext* m_threadContext and a int8_t*
    // machineCode pointer. The threadContext is loaded into the kContextPointerReg, and the code pointer is loaded into
    // Reg 0. As Lightening re-uses the C-calling convention stack register JIT_SP as a general-purpose register, I have
    // taken some care to ensure that GPR(2)/Reg 0 is not the stack pointer on any of the supported architectures.
    jit.loadCArgs2(JIT::kContextPointerReg, JIT::Reg(0));
    // Save the C stack pointer, this pointer is *not* tagged as it does not point into Hadron-allocated heap.
    jit.stxi_w(offsetof(ThreadContext, cStackPointer), JIT::kContextPointerReg, jit.getCStackPointerRegister());
    // Restore the Hadron frame pointer.
    jit.ldxi_w(JIT::kFramePointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, framePointer));
    // Restore the Hadron stack pointer.
    jit.ldxi_w(JIT::kStackPointerReg, JIT::kContextPointerReg, offsetof(ThreadContext, stackPointer));
    // Jump into the calling code.
    jit.jmpr(JIT::Reg(0));

    m_exitTrampoline = jit.addressToFunctionPointer(jit.address());

    // Save frame and stack pointers back to thread context
    jit.stxi_w(offsetof(ThreadContext, framePointer), JIT::kContextPointerReg, JIT::kFramePointerReg);
    jit.stxi_w(offsetof(ThreadContext, stackPointer), JIT::kContextPointerReg, JIT::kStackPointerReg);

    // Restore the C stack pointer.
    jit.ldxi_w(jit.getCStackPointerRegister(), JIT::kContextPointerReg, offsetof(ThreadContext, cStackPointer));
    jit.leaveABI(align);
    jit.ret();

    assert(!jit.hasJITBufferOverflow());
    size_t trampolineSize = 0;
    auto entryAddr = jit.end(&trampolineSize);
    m_entryTrampoline = reinterpret_cast<void (*)(ThreadContext*, const int8_t*)>(
            jit.addressToFunctionPointer(entryAddr));
    jitArray.resize(m_threadContext.get(), trampolineSize);

    m_threadContext->exitMachineCode = reinterpret_cast<int8_t*>(m_exitTrampoline);
    return true;
}

} // namespace hadron