#ifndef SRC_HADRON_RUNTIME_HPP_
#define SRC_HADRON_RUNTIME_HPP_

#include "hadron/library/ArrayedCollection.hpp"
#include "hadron/library/Interpreter.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <string_view>
#include <string>
#include <unordered_set>

namespace hadron {

class ErrorReporter;
class Heap;
struct ThreadContext;
class VirtualMachine;

// Owns all of the objects required to compile and run SC code, including the Heap, ThreadContext, and ClassLibrary.
class Runtime {
public:
    Runtime() = delete;
    explicit Runtime(bool debugMode);
    ~Runtime();

    // Finalize members in ThreadContext, bootstraps class library, initializes language globals needed for the
    // Interpreter.
    bool initInterpreter();

    // Adds the default class library directory and the Hadron HLang class directory to the search path.
    void addDefaultPaths();

    // Adds a directory to the list of directories to scan for library classes.
    void addClassDirectory(const std::string& path);

    // Scans any directories provided for .sc files, provides those files to the class library for scanning.
    bool scanClassFiles();

    // Scan a class input string for class definitions.
    bool scanClassString(std::string_view input, std::string_view filename);

    bool finalizeClassLibrary();

    // Compile and run the provided input string, returning the results.
    Slot interpret(std::string_view code);
    std::string slotToString(Slot s);

    ThreadContext* context() { return m_threadContext.get(); }

private:
    bool buildThreadContext();
    bool buildTrampolines();

    // We keep the normalized paths in a set to prevent duplicate additions of the same path.
    std::unordered_set<std::string> m_libraryPaths;

    std::shared_ptr<Heap> m_heap;
    std::unique_ptr<ThreadContext> m_threadContext;

    // The interpreter instance.
    library::Interpreter m_interpreter;

    // We JIT entry and exit machine code into this array, so we maintain a reference to it.
    library::Int8Array m_trampolines;
    // Saves registers, initializes thread context and stack pointer registers, and jumps into the machine code pointer.
    void (*m_entryTrampoline)(ThreadContext* context, const int8_t* machineCode);

    std::unique_ptr<VirtualMachine> m_virtualMachine;
};

} // namespace hadron

#endif // SRC_HADRON_RUNTIME_HPP_