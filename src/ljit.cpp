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

    // disable blocking writes for jit memory regions
    pthread_jit_write_protect_np(false);

    // Mark the beginning of a new JIT
    jit_begin(jitState, static_cast<uint8_t*>(jitMem), 4 * 1024);
    // Capture the address at the start of the jit.
    jit_pointer_t jitStartAddress = jit_address(jitState);

    // args are state, number of registers used, number of float registers used, stack space to reserve
    size_t stackAlign = jit_enter_jit_abi(jitState, 3, 0, 0);

/*
         prolog
         allocai 0
label_0: arg label_0
         alias %vr0
         movi %vr0, 23
         alias %vr1
         getarg_w %vr1, label_0
         stxi_i 0x4, %vr1, %vr0
         unalias %vr0
         alias %vr0
         movi %vr0, 2
         str_i %vr1, %vr0
         unalias %vr0
         unalias %vr1
         reti 1
         epilog
*/

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