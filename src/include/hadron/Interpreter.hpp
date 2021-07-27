#ifndef SRC_HADRON_INCLUDE_INTERPRETER_HPP_
#define SRC_HADRON_INCLUDE_INTERPRETER_HPP_

#include "hadron/JITMemoryArena.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <string_view>

namespace hadron {

class ErrorReporter;
struct Function;
struct ThreadContext;

// Trying to loosely follow the Interpreter API in SC
class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    // Setup entry and exit trampolines, other infrastructure needed to compile and run sclang code.
    bool setup();
    // Tear down resources and prepare for exit.
    void teardown();

    // Compile the provided code string and return a function. TODO: We want to allow mixing of interpreted code,
    // class definitions, and class extensions. Is there a std::variant<> return pattern, or would it make more sense
    // to add new methods compileMixed or something that support this? Also, given that in Hadron compilation happens on
    // a separate thread, would it make sense to also have some sort of closure call option too, like took a function or
    // something to call back when compilation was complete?
    std::unique_ptr<Function> compile(std::string_view code);
    std::unique_ptr<Function> compileFile(std::string path);

    // Runs the provided block on the calling thread. Constructs a new ThreadContext, including a stack. On normal exit,
    // pulls the result from the stack and returns it.
    Slot run(Function* func);

private:
    // Sets up the return address pointer in the stack and jumps into machine code.
    void enterMachineCode(ThreadContext* context, const uint8_t* machineCode);

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<JITMemoryArena> m_jitMemoryArena;

    // Saves registers, initializes thread context and stack pointer registers, and jumps into the machine code pointer.
    void (*m_entryTrampoline)(ThreadContext* context, const uint8_t* machineCode);
    // Restores registers and returns control to C++ code.
    void (*m_exitTrampoline)();

    JITMemoryArena::MCodePtr m_trampolines;
};

} // namespace hadron

#endif // SRC_HADRON_INCLUDE_INTERPRETER_HPP_