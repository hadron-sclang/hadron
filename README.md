# Hadron

Hadron is a re-implementation of the [SuperCollider](https://supercollider.github.io/) language interpreter, designed to
Just-In-Time (JIT) compile SuperCollider code into machine code instructions. Hadron is in early development stage and
is not ready for use, but will hopefully one day be useful for the SuperCollider community.

The goals of Hadron are to make the SuperCollider language a *faster*, more *extensible*, and more *usable* programming
language than ever before.

## Execution Speed

Hadron compiles blocks of SuperCollider code into machine code for execution directly on the host microprocessor.
Balancing against compilation speed, it makes a best effort to pack local variables into processor registers, and to
deduce types of variables to "narrow down" the possible types of values to inline and optimize code at compile time as
much as can be done on a dynamically-typed programming language.

However, raw speed on its own may not represent a significant value to the SC community. Typical uses of SuperCollider
rely on short snippets of code executing in a coordinated fashion to control behavior on the SC audio synthesis server
via OSC messages. But, having a faster language interpreter opens up several other possibilities for SC development that
are potentially exciting.

For example, typical SC deployments run the language and audio server on the same computer. In this context, every CPU
cycle taken up by the language represents a CPU cycle not available for audio synthesis. Having a faster language might
increase the usability of SC on smaller and more inexpensive hardware platforms.

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
system libraries to provide system functions such as filesystem, network, and peripheral access. Hadron will extend the
SuperCollider grammar with type hinting and function signatures to enable calling C and C++ functions *directly* from
the language runtime. The Hadron C API will hopefully enable a wide variety of interoperability and extensibility
options, while also allowing the core library to remain small, delegating many functions (such as UI and MIDI
frameworks) to extensions.

## Usability

One of the great strengths of the SuperCollider user base is its diversity. For many SuperCollider users, the SC
programming language may represent their first encounter with computer programming. In order to minimize confusion and
difficulty learning that first language, it is important that the language be as usable as possible.

Error messages from the language should provide clear, actionable feedback to the user, and could also be translated
into other human languages, allowing speakers of languages other than English greater access to the software. Support
for standards such as [Language Server Protocol](https://microsoft.github.io/language-server-protocol/) allow for a
richer integration with editing and performance tools. Lastly, interactive debugger and profiling support should empower
SuperCollider users with insight into the performance and behavior of their programs, greatly accelerating learning and
development.
