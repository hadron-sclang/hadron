# Hadron JIT Stack Frame, Calling Convention and Message Dispatch

We reserve the first general-purpose register, GPR0, to always contain a pointer to the C struct `ThreadContext`. This
struct is created per-thread and serves as the primary data structure for interaction between machine code and the rest
of Hadron.

## Calling Convention

JIT-compiled machine code should return two pointers, one with a preamble/epilog that can set up/tear down the stack
frame according to the Hadron convention, so going from C calling convention to Hadron and then back, and then the
second, inner function that assumes Hadron calling convention is already in place.

## Message Dispatch

Message dispatch within Hadron needs to be runtime reconfigurable, meaning that it should be possible to patch the
dispatch table during normal program execution, granted suitable synchronization between threads. The job of dispatch is
to find a match with two hashes, the selector and the target Class.

The tables are designed to be efficiently accessed from JITted code without having to transition back out to C++ code
between each function call. We use a simple least significant bits hashing strategy. The LSBHashTable struct contains
an array of structs. The array is always a power of two in size and includes a bitmask for hashes that limit them to
indices of an array of that size, meaning that if the array is currently 256 entries in size than the bitmask will be
0xff and the index of all elements in the table is determined soley by their first 8 bits.

