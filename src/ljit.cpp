#include <errno.h>
#include <iostream>
#include <pthread.h>  // for apple jit magic
#include <string.h>
#include <sys/mman.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc99-extensions"
extern "C" {
#include "lightening.h"
}
#pragma GCC diagnostic pop

/*
There are entry and exit trampolines do go from Scheme VM (written in C) code to JITted code.

JITted code can have its own mmap'ed stack, or some fancy bullshit like that. Might be good to keep track of things
like stack overruns. Dispatch from JIT-to-JIT - runtime context. Garbage Collector lives there, dispatch table, saved
stack pointer and frame pointer (for trampoline).

So.. how could dispatch work? Thunk out to C to find the right function, then back?

For JITted code we know what methods to dispatch, could we JIT a web of checks on Target? Requires knowing about the
targets in advance. Maybe like a fast check on type, for known things, for a straight shot across from JIT to JIT code.
compiled C would need a trampoline out anyway, too.

Stack setup for JIT function calls (borrowing liberally from Guile stack setup)

Stack grows down, so sp < fp always.

   +------------------------------+
   | Machine return address (mRA) |
   +==============================+ <- fp (real stack pointer)
   | Argument 0                   |
   +------------------------------+
   | Argument 1                   |
   +------------------------------+
   | ...                          |
   +------------------------------+
   | Argument N-1                 |
   \------------------------------/ <- sp (reserved register)

Guile reserves a register to use as the stack pointer, leaving the *real* stack pointer as the frame pointer, we do the
same.

Arguments are always pushed in order and are all Slots. Callee can determine the number of arguments from the
fp - sp calculation. Register spill storage comes next, as that is known size.

Question - do we actually need to reserve a register for a stack pointer? We do support va_args functions but the number
of arguments is always included, meaning that arguments can be located on the stack without an sp. There's a single
return value that is also a Slot, so can add that on top of the other arguments. Then there's register spill space,
which if there's one slot for every virtual register all those locations are known at JIT time too:

   +------------------------------+
   | Machine return address (mRA) |
   +==============================+ <- frame pointer
   | Argument 0                   |
   +------------------------------+
   | Argument 1                   |
   +------------------------------+
   | ...                          |
   +------------------------------+
   | Argument N-1                 |
   +------------------------------+
   | Return Value                 |
   +------------------------------+
   | Virtual Register Spill 0     |
   +------------------------------+
   | Virtual Register Spill 1     |
   +------------------------------+
   | ...                          |
   +------------------------------+
   | Virtual Register Spill N-1   |
   +------------------------------+

There are two functions to dispatch a message, one for an ordered list of arguments and one for key/value pairs. They
both trampoline out of JIT code and back to C (I think) so that's an important consideration.

When type is known it could be possible to "inline" right to the correct function.

 */

int main(int /* argc */, char** /* argv */) {
    if (!init_jit()) {
        std::cerr << "Lightening JIT failed to init." << std::endl;
        return -1;
    }

    jit_state_t* jitState = jit_new_state(malloc, free);
    if (!jitState) {
        std::cerr << "Lightening JIT didn't make the new state." << std::endl;
        return -1;
    }

    // next call is to jit_begin, which seems to want a chunk of memory and a size (in which to JIT)
//    void* jitMem = mmap(nullptr, 4 * 1024, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    // begin apple-specific magic -- added MAP_JIT
    void* jitMem = mmap(nullptr, 4 * 1024, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_JIT | MAP_PRIVATE | MAP_ANONYMOUS,
        -1, 0);

    if (jitMem == MAP_FAILED) {
        int mmapErr = errno;
        std::cerr << "mmap failed: " << mmapErr << " " << strerror(mmapErr) << std::endl;
        return -1;
    }

    // disable blocking writes for jit memory regions - this is per-thread, so worth verifying if we can lock in
    // one thread and write, while executing in another.
    pthread_jit_write_protect_np(false);

    // Mark the beginning of a new JIT
    jit_begin(jitState, static_cast<uint8_t*>(jitMem), 4 * 1024);
    // Capture the address at the start of the jit.
    jit_pointer_t jitStartAddress = jit_address(jitState);

    // args are state, number of registers used, number of float registers used, stack space to reserve
    size_t stackAlign = jit_enter_jit_abi(jitState, 3, 0, 0);

    int value;

    jit_movi(jitState, JIT_GPR(0), reinterpret_cast<long>(&value));
    jit_movi(jitState, JIT_GPR(1), 23);
    jit_str_i(jitState, JIT_GPR(0), JIT_GPR(1));
    jit_leave_jit_abi(jitState, 3, 0, stackAlign);
    jit_ret(jitState);

    void (*function)() = jit_address_to_function_pointer(jitStartAddress);

    // enable blocking writes for jit regions, to allow to execute that region
    pthread_jit_write_protect_np(true);

    function();

    std::cout << "value: " << value << std::endl;

    // Teardown
    munmap(jitMem, 4 * 1024);
    jit_destroy_state(jitState);

    return 0;
}