#ifndef SRC_INCLUDE_HADRON_COMPILER_HPP_
#define SRC_INCLUDE_HADRON_COMPILER_HPP_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

namespace hadron {

class ErrorReporter;
struct Function;
class JITMemoryArena;
struct ThreadContext;

// Owns the threads responsible for compilation of sclang code. On MacOS, configures these compilation threads to write
// to executable memory, which because of mutual exclusion reasons mean that they may not execute that memory until
// reclassified.
class Compiler {
public:
    Compiler(std::shared_ptr<ErrorReporter> errorReporter);
    ~Compiler();

    // Start the compiler threads, they will block until input is provided. The Compiler needs at least one thread. If
    // zero is provided as an argument the compiler will start a default of max(1, (hardwareThreads / 2) - 1)) threads.
    bool start(size_t numberOfThreads = 0);
    void stop();

    // Enqueues a request for compilation of the provided string_view to the worker threads. The string_view must
    // remain pointing to valid memory until the closure is called. The result callback will be called with a valid
    // pointer to a Function or nullptr if compilation failed.
    void compile(std::string_view code, std::function<void(std::unique_ptr<Function>)> func);

    JITMemoryArena* jitMemoryArena() { return m_jitMemoryArena.get(); }

private:
    void compilerThreadMain(size_t threadNumber);
    void asyncCompile(std::string_view code, std::function<void(std::unique_ptr<Function>)> func);

    std::unique_ptr<JITMemoryArena> m_jitMemoryArena;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::atomic<bool> m_quit;
    std::vector<std::thread> m_compilerThreads;

    // Protects m_jobQueue
    std::mutex m_jobQueueMutex;
    std::condition_variable m_jobQueueCondition;
    std::deque<std::function<void()>> m_jobQueue;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_COMPILER_HPP_