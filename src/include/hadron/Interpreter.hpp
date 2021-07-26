#ifndef SRC_HADRON_INCLUDE_INTERPRETER_HPP_
#define SRC_HADRON_INCLUDE_INTERPRETER_HPP_

#include "hadron/JITMemoryArena.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <string_view>

// Sigh. The goal of this PR is to switch over to lightening. So adding functionality is a bit beside the point. Method
// Dispatch is off the table, although clearly that seems to be what's coming next. Not sure if this Interpreter file is
// the right call yet. I think what led me here was the realization that with the new JIT stuff that class goes from a
// single-use ephemeral class to a re-usable kind of thing. *and* it starts to require a partition between threads that
// JIT compile and threads that execute the compiled code.
// CompilerContext should evolve to become a Compiler which owns a parameterizable number of compilation threads, but
// at least one. Each thread owns its own instance of the toolchain. Worth some thought about error reporting, how to
// structure that, multithreaded error reporting seems reasonable particularly given we'll need a runtime error
// reporting trampoline (need to bring in Error class soon). Compiler owns the JITMemoryArena object. Maybe Interpreter
// owns a Compiler, although it's not the only thing that can own a Compiler. Then Interpreter can mark the calling
// thread as suitable for executing JIT code, or provide a function to do that. It does not yet have to presume a
// threading model for hadron runtime, that's for another day.

// a) CompilerContext => Compiler
// b) Compilation threads for Compiler
// c) Add JITMemoryArena in, add realloc
// d) refactor toolchain to be re-usable, and tests to reflect
// e) refactor CodeGeneration, Rendering, JIT, Function
// ** compilation **
// f) hadronc testing

namespace hadron {

class Compiler;
class ErrorReporter;
struct Function;
struct ThreadContext;

// Trying to loosely follow the Interpreter API in SC
class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    // Starts the Compiler threads, setup entry and exit trampolines, other infrastructure needed to compile and
    // run sclang code.
    bool start();
    // Tear down resources and prepare for exit.
    void stop();

    // Compile the provided code string and return a function. TODO: We want to allow mixing of interpreted code,
    // class definitions, and class extensions. Is there a std::variant<> return pattern, or would it make more sense
    // to add new methods compileMixed or something that support this? Also, given that in Hadron compilation happens on
    // a separate thread, would it make sense to also have some sort of closure call option too, like took a function or
    // something to call back when compilation was complete?
    std::unique_ptr<Function> compile(std::string_view code);
    std::unique_ptr<Function> compileFile();

    // Runs the provided block on the calling thread. Constructs a new ThreadContext, including a stack. On normal exit,
    // pulls the result from the stack and returns it.
    Slot run(Function* func);

private:
    // Sets up the return address pointer in the stack and jumps into machine code.
    void enterMachineCode(ThreadContext* context, const uint8_t* machineCode);

    std::shared_ptr<ErrorReporter> m_errorReporter;
    std::unique_ptr<Compiler> m_compiler;

    // Saves registers, initializes thread context and stack pointer registers, and jumps into the machine code pointer.
    void (*m_entryTrampoline)(ThreadContext* context, const uint8_t* machineCode);
    // Restores registers and returns control to C++ code.
    void (*m_exitTrampoline)();

    JITMemoryArena::MCodePtr m_trampolines;
};

} // namespace hadron

#endif // SRC_HADRON_INCLUDE_INTERPRETER_HPP_