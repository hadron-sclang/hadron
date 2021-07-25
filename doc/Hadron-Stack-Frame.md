# Hadron JIT Stack Frame, Calling Convention and Message Dispatch

## Hadron Calling Convention

Hadron reserves two registers, GPR0 and GPR1, while running any machine code. GPR0 will always point to the
thread-specific ThreadContext structure. GPR1 serves as the stack pointer. Hadron maintains its own stack, which is
slot-aligned, meaning every entry is expected to be a 16-byte slot.

Stacks grow down, so pushing something onto the Hadron stack means *decrementing* the stack pointer. Thread context also
maintains a *frame* pointer, which points at the bottom of the stack frame for the current calling code. At function
entry point the stack is laid out as follows:

| frame pointer | contents               | stack pointer |
|---------------|------------------------|---------------|
| `fp` + 3      | Caller Frame Pointer   |               |
| `fp` + 2      | Caller Stack Pointer   |               |
| `fp` + 1      | Caller Return Address  |               |
| `fp`          | Return Value Slot      |               |
|               | < dispatch work area > |               |
|               | Argument 0             | `sp` + n      |
|               | Argument 1             | `sp` + n - 1  |
|               |  ...                   |  ...          |
|               | Argument n - 1         | `sp` + 1      |
|               | < register spill area> | `sp`          |

Because the dispatch work area can occupy a variable number of slots, only the return value is located using the frame
pointer, and the arguments are located relative to the stack pointer. The dispatch work area allows method dispatch to
happen without rewriting the stack or making a second copy of the arguments, first for dispatch and then for actual
function execution.

## Method Dispatch

To dispatch a method the caller must first determine the callee frame pointer location and make room there for the
previous frame pointer storage, the caller return address, and the return value slot, initalized to `nil`. Then the
dispatch area is setup as follows:

| frame pointer | contents                | stack pointer |
|---------------|-------------------------|---------------|
| `fp`          | Return Value Slot       |               |
| `fp` + 1      | Target Name Hash        |               |
| `fp` + 2      | Selector Name Hash      |               |
| `fp` + 3      | Number of Keyword Args  |               |
| `fp` + 4      | Number of In-order Args |               |
| `fp` + 5      | Keyword Arg 0 Keyword   |               |
| `fp` + 6      | Keyword Arg 0 Value     |               |
|  ...          | ...                     |               |
|               | Argument 0              | `sp` + n      |

With the hashes and argument counts, followed by the keyword/value argument pairs, taking up the space in the dispatch
work area. Note that `fp` and `sp` are already set up for the function call. The dispatch code can find the target
function, append any arguments from the inorder list that were ommitted from the originating code by consulting the
defaults list (and fixing up `sp`), and then override any values in the argument stack by iterating through the provided
keyword arguments. Then the dispatch jumps directly into the callee code.

As Hadron does not use the application stack there are no `call` or `ret` instructions, only `jmp` and stack
manipulation. To return from machine code it is a matter of jumping back to the caller return address provided at `fp` +
1. The caller can then use the frame pointer as the address of the ephemeral return value slot, restore the frame and
stack pointers, and with all virtual registers currently marked as "spilled" the register fitting code should lazily
restore use of the needed registers in continuing code.

## Jumping into and out of Hadron Machine Code

It is the responsibility of the C calling code to establish the Hadron stack frame according to the calling convention.
As the Hadron stack is allocated heap memory manipulation of the stack from C code is possible, although only advisable
if done on the thread owning the ThreadContext object the stack lives within.

The *trampoline* into Hadron will save all the callee-save registers, as due to the nature of dynamic dispatch it is not
possible to predict which registers might be modified by Hadron machine code. Then it should set up the frame pointer
and initialize the stack pointer, and ThreadContext pointer registers. The return address and also the `exitMachineCode`
pointer within the ThreadContext should be set to return to the trampoline exit code.

Hadron machine code may exit back to C code for a variety of reasons. If the base function called from C code has
returned, the `machineCodeStatus` value is set to `kOk`, and the trampoline detects that the frame pointer is what it
was set to on machine code entry, this counts as a normal exit from machine code. But the machine code could exit with
state still on the stack, either in the event of runtime error or exception, or resulting from the need to call an
intrinsic function. In these cases the trampoline should execute whatever C code was requested by the machine code,
possibly modifying the Hadron stack in the process, and then begin the process for entry back into machine code again.
