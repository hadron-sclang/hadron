# Hadron

Hadron is a re-implementation of the [SuperCollider](https://supercollider.github.io/) language interpreter, designed to
Just-In-Time (JIT) compile SuperCollider code into native machine instructions using [LLVM](https://llvm.org/). It is in
early development stage and is not ready for use, but will hopefully one day be useful for the SC community.

The goals of Hadron are to make the SuperCollider language a *faster*, more *extensible*, and more *usable* programming
language than ever before.

## Execution Speed

The current SuperCollider language compiles programs into a series of bytecodes that are interpreted as individual
instructions on a virtual machine. Experience in working in JavaScript engines has shown that moving to a JIT
architecture typically allows for a significant speedup in program execution. It is possible in ideal cases on synthetic
benchmarks for JIT-compiled code to approach speeds near that of compiled C code.

However, raw speed on its own may not represent a significant value to the SC community. Typical uses of SuperCollider
rely on short snippets of code executing in a coordinated fashion to control behavior on the SC audio synthesis server
via OSC messages. But, having a faster language interpreter opens up several other possibilities for SC development that
are potentially exciting.

For example, typical SC deployments these days run the language and audio server on the same computer. In this context,
every CPU cycle taken up by the language represents a CPU cycle not availble for audio synthesis. Having a faster
language might increase the usability of SC on smaller and more inexpensive hardware platforms.

Furthermore, a faster language means that more of SuperCollider could be implemented in SuperCollider itself. A typical
SC language development best practice has been to implement speed-critical features as *intrinsics* in the language,
special-case functions that call out to compiled C++ code to complete the "heavy lifting" required by the code. Granted,
this is not the only reason to add an intrinsic to the language, and is arguably not even the majority reason (see
extensibility below) for adding intrinsics, but the more of SuperCollider's fundamentals that can be written in the SC
language, the more understandable, debug-able, maintainable the language will be.

Speed increases SuperCollider's *reach*, and thus hopefully speed expands the creative possibilities of SuperCollider
itself.

## Extensibility and Interoperability

An important feature of any high-level programming language like SuperCollider is the ability to link with lower-level
system libraries to provide system functions such as filesystem, network, and peripheral access. Hadron depends on LLVM
for native machine code generation, and another important function that LLVM can offer is to enable lazy linking to
libraries. This, with supporting engineering, should allow calls to C and C++-based libraries from SuperCollider code
*without* recompilation of either the interpreter or the class library.

C Bindingsz
WebAssembly
AoT

## Usability

One of the great strengths of the SuperCollider user base is its diversity. For many SuperCollider users, the SC
programming language may represent their first encounter with computer programming. In order to minimize confusion and
difficulty learning that first language, it is important that the language be as usable as possible.

Error messages from the language should provide clear, actionable feedback to the user, and could also be translated
into other human languages, allowing speakers of languages other than English greater access to the software. Support
for standards such as [Language Server Protocol](https://microsoft.github.io/language-server-protocol/) allow for a
richer integration with editing and performance tools. Lastly, interactive debugger and profiling support, enabled in
part by integration with LLVM's rich toolset, should empower SuperCollider users with insight into the performance and
behavior of their programs, greatly accelerating learning and development.
