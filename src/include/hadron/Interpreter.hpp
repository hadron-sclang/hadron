#ifndef SRC_HADRON_INCLUDE_INTERPRETER_HPP_
#define SRC_HADRON_INCLUDE_INTERPRETER_HPP_

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

struct Function;

// Trying to loosely follow the Interpreter API in SC
class Interpreter {
public:
    // Compile the provided code string and return a function. TODO: We want to allow mixing of interpreted code,
    // class definitions, and class extensions. Is there a std::variant<> return pattern, or would it make more sense
    // to add new methods compileMixed or something that support this? Also, given that in Hadron compilation happens on
    // a separate thread, would it make sense to also have some sort of closure call option too, like took a function or
    // something to call back when compilation was complete?
    std::unique_ptr<Function> compile(std::string_view code);


    std::unique_ptr<Function> compileFile();
    Slot executeFile();

};

} // namespace hadron

#endif // SRC_HADRON_INCLUDE_INTERPRETER_HPP_