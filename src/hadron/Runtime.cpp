#include "hadron/Runtime.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Heap.hpp"
#include "hadron/LighteningJIT.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"
#include "internal/FileSystem.hpp"

#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Core/Kernel.hpp"
#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/SequenceableCollection.hpp"
#include "schema/Common/Collections/ArrayedCollection.hpp"
#include "schema/Common/Collections/Array.hpp"

#include "spdlog/spdlog.h"

#include <array>

namespace {
static const std::array<const char*, 1> kClassFileAllowList{
    "Object.sc"
};
}

namespace hadron {

Runtime::Runtime(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter),
    m_heap(std::make_shared<Heap>()),
    m_threadContext(std::make_unique<ThreadContext>()),
    m_classLibrary(std::make_unique<ClassLibrary>(errorReporter)) {
    m_threadContext->heap = m_heap;
}

Runtime::~Runtime() {}

bool Runtime::initialize() {
    if (!buildTrampolines()) return false;
    if (!buildThreadContext()) return false;
    if (!recompileClassLibrary()) return false;

    return true;
}

bool Runtime::recompileClassLibrary() {
    auto classLibPath = findSCClassLibrary();
    SPDLOG_INFO("Starting Class Library compilation for files at {}", classLibPath.c_str());

    for (auto& entry : fs::recursive_directory_iterator(classLibPath)) {
        const auto& path = entry.path();
        if (!fs::is_regular_file(path) || path.extension() != ".sc")
            continue;
        // For now we have an allowlist in place, to control which SC class files are parsed, and we lazily do an
        // O(n) search in the array for each one.
        bool classFileAllowed = false;
        for (size_t i = 0; i < kClassFileAllowList.size(); ++i) {
            if (path.filename().compare(kClassFileAllowList[i]) == 0) {
                classFileAllowed = true;
                break;
            }
        }
        if (!classFileAllowed) {
            // SPDLOG_WARN("Skipping compilation of {}, not in class file allow list.", path.c_str());
            continue;
        }

        SPDLOG_INFO("Class Library compiling '{}'", path.c_str());
        m_classLibrary->addClassFile(m_threadContext.get(), path);
    }
    return true;
}

bool Runtime::buildTrampolines() {
    LighteningJIT::markThreadForJITCompilation();
    size_t jitBufferSize = 0;
    library::Int8Array* jitArray = m_heap->allocateJIT(Heap::kSmallObjectSize, jitBufferSize);
    m_heap->addToRootSet(jitArray);
    LighteningJIT jit(m_errorReporter);
    jit.begin(reinterpret_cast<uint8_t*>(jitArray) + sizeof(library::Int8Array),
            jitBufferSize - sizeof(library::Int8Array));
    auto align = jit.enterABI();
    // Loads the (assumed) two arguments to the entry trampoline, ThreadContext* m_threadContext and a uint8_t* machineCode
    // pointer. The threadContext is loaded into the kContextPointerReg, and the code pointer is loaded into Reg 0. As
    // Lightening re-uses the C-calling convention stack register JIT_SP as a general-purpose register, I have taken
    // some care to ensure that GPR(2)/Reg 0 is not the stack pointer on any of the supported architectures.
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
    jitArray->_sizeInBytes = trampolineSize + sizeof(library::Int8Array);
    SPDLOG_INFO("Runtime built JIT trampoline at {} bytes out of {} max.", trampolineSize, jitBufferSize);

    return true;
}

bool Runtime::buildThreadContext() {
    library::Process* process = reinterpret_cast<library::Process*>(m_heap->allocateObject(library::kProcessHash,
            sizeof(library::Process)));
    m_heap->addToRootSet(process);
    process->classVars = m_heap->allocateObject(library::kArrayHash, sizeof(library::Array));
    process->interpreter = Slot();
    process->curThread = Slot();
    process->mainThread = Slot();
    process->schedulerQueue = Slot();
    process->nowExecutingPath = Slot();
    return true;
}

void Runtime::enterMachineCode(const uint8_t* machineCode) {
    // Set machine return address as the exit trampoline into Hadron stack frame.
    *(m_threadContext->framePointer) = m_threadContext->framePointer;
    --(m_threadContext->framePointer);
    *(m_threadContext->framePointer) = m_threadContext->stackPointer;
    --(m_threadContext->framePointer);
    *(m_threadContext->framePointer) = reinterpret_cast<uint8_t*>(m_exitTrampoline);
    --(m_threadContext->framePointer);

    // Initialize return value.
    *(m_threadContext->framePointer) = Slot();
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