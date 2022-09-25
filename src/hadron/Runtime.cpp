#include "hadron/Runtime.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/Generator.hpp"
#include "hadron/Heap.hpp"
#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Object.hpp"
#include "hadron/library/Schema.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/library/Thread.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SourceFile.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"
#include "internal/FileSystem.hpp"

#include "spdlog/spdlog.h"
#include <memory>

namespace hadron {

Runtime::Runtime(): m_heap(std::make_shared<Heap>()), m_threadContext(std::make_unique<ThreadContext>()) {
    m_threadContext->heap = m_heap;
    m_threadContext->symbolTable = std::make_unique<SymbolTable>();
    m_threadContext->classLibrary = std::make_unique<ClassLibrary>();
    m_threadContext->generator = std::make_unique<Generator>();
}

Runtime::~Runtime() { }

bool Runtime::initInterpreter() {
    if (!buildThreadContext())
        return false;
    m_threadContext->classLibrary->bootstrapLibrary(m_threadContext.get());
    m_interpreter = library::Interpreter::alloc(m_threadContext.get());
    m_interpreter.initToNil();

    return true;
}

void Runtime::addDefaultPaths() {
    addClassDirectory(findSCClassLibrary().string());
    addClassDirectory(findHLangClassLibrary().string());
}

void Runtime::addClassDirectory(const std::string& path) { m_libraryPaths.emplace(fs::absolute(path).string()); }

bool Runtime::scanClassFiles() {
    for (const auto& classLibPath : m_libraryPaths) {
        for (auto& entry : fs::recursive_directory_iterator(classLibPath)) {
            const auto& path = fs::absolute(entry.path());
            if (!fs::is_regular_file(path) || path.extension() != ".sc")
                continue;

            auto sourceFile = std::make_unique<SourceFile>(path.string());
            if (!sourceFile->read()) {
                return false;
            }

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

bool Runtime::finalizeClassLibrary() { return m_threadContext->classLibrary->finalizeLibrary(m_threadContext.get()); }

Slot Runtime::interpret(std::string_view code) {
    Generator::markThreadForJITCompilation();

    // TODO: refactor to avoid this needless copy
    auto codeString = library::String::fromView(m_threadContext.get(), code);
    auto function = m_interpreter.compile(m_threadContext.get(), codeString);

    if (!function) {
        return Slot::makeNil();
    }

    // Convention is caller pushes, callee pops.
    auto callerFrame = library::Frame::alloc(m_threadContext.get());
    callerFrame.initToNil();

    auto calleeFrame =
        library::Frame::alloc(m_threadContext.get(), std::max(function.def().prototypeFrame().size() - 1, 0));
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

    Generator::markThreadForJITExecution();

    SCMethod scMethod = reinterpret_cast<SCMethod>(function.def().code().getRawPointer());
    auto result = scMethod(m_threadContext.get(), calleeFrame.instance());
    return Slot::makeFromBits(result);
}

std::string Runtime::slotToString(Slot s) {
    switch (s.getType()) {
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
        auto className = library::Symbol(m_threadContext.get(), Slot::makeSymbol(s.getPointer()->className));
        return fmt::format("a {}", className.view(m_threadContext.get()));
    }

    case TypeFlags::kNilFlag:
        return "nil";

    case TypeFlags::kSymbolFlag: {
        auto symbol = m_threadContext->symbolTable->lookup(s.getSymbolHash());
        return std::string(symbol);
    }

    default:
        return "UNKNOWN TYPE IN SLOT_TO_STRING";
    }
}

bool Runtime::buildThreadContext() {
    m_threadContext->symbolTable->preloadSymbols(m_threadContext.get());
    m_threadContext->thisProcess = library::Process::alloc(m_threadContext.get()).instance();
    m_threadContext->thisThread = library::Thread::alloc(m_threadContext.get()).instance();
    return true;
}

} // namespace hadron