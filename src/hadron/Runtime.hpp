#ifndef SRC_HADRON_RUNTIME_HPP_
#define SRC_HADRON_RUNTIME_HPP_

#include "hadron/library/Interpreter.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <string_view>
#include <string>
#include <unordered_set>

namespace hadron {

class Heap;
struct ThreadContext;

// Owns all of the objects required to compile and run SC code, including the Heap, ThreadContext, and ClassLibrary.
class Runtime {
public:
    Runtime();
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

    ThreadContext* context() const { return m_threadContext.get(); }

private:
    bool buildThreadContext();

    // We keep the normalized paths in a set to prevent duplicate additions of the same path.
    std::unordered_set<std::string> m_libraryPaths;

    std::shared_ptr<Heap> m_heap;
    std::unique_ptr<ThreadContext> m_threadContext;

    // The interpreter instance.
    library::Interpreter m_interpreter;
    library::Process m_process;
};

} // namespace hadron

#endif // SRC_HADRON_RUNTIME_HPP_