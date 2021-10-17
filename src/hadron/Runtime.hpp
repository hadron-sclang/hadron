#ifndef SRC_HADRON_RUNTIME_HPP_
#define SRC_HADRON_RUNTIME_HPP_

#include <memory>

namespace hadron {

class ClassLibrary;
class ErrorReporter;
class Heap;
struct ThreadContext;

// Owns all of the objects required to compile and run SC code, including the Heap, ThreadContext, and ClassLibrary.
class Runtime {
public:
    explicit Runtime(std::shared_ptr<ErrorReporter> errorReporter);
    Runtime() = delete;
    ~Runtime();

    // sclang langauge config file? Some interpreter business?

    // First time compilation of class library, then create Kernel runtime objects like Interpreter and Process.
    bool initialize();

    // Lazy recompilation of class library?
    bool recompileClassLibrary();

private:
    void findRunningBinary();

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::shared_ptr<Heap> m_heap;
    std::unique_ptr<ThreadContext> m_threadContext;
    std::unique_ptr<ClassLibrary> m_classLibrary;
};

} // namespace hadron

#endif // SRC_HADRON_RUNTIME_HPP_