# Hadron Requirements

## Goals and Philosophy

Hadron is a next generation implementation of the SuperCollider programming language. As such it should generally meet
or exceed the performance and usability characteristics of the current implementation.

SuperCollider remains a unique creative tool in a crowded field of electronic music production software, likely at least
in part because of the programming-centric approach. For many users the SuperCollider language is their first
programming language and may be the only programming language they learn. Hadron aims to improve both the usability of
the language as well as its utility, therefore radically expanding both the user community and the creative potential of
SuperCollider.

## JiT/REPL/Live Coding and AoT Compilation Models

Hadron could support ahead-of-time compilation, but one of the built-in components of Hadron is the interpreter, so
perhaps AoT would be more about distributing the Interpreter and core runtime along with the source code, allowing
programs to be destributed that execute the runtime and run the described startup code.

## Type System

Adds type declaration as an extension to SuperCollider syntax.

### Multiple Dispatch

Similar to the [Julia](https://julialang.org/) programming language we could use [Multiple
Dispatch](https://en.wikipedia.org/wiki/Multiple_dispatch) for efficient function dispatch.

## Execution Speed and Realtime Availability

Intuition suggests that most SuperCollider programs don't necessarily saturate CPU or memory bandwidth all of the time,
but rather have small bits of code that need to be executed with precision timing. The current implementation has some
discussion surrounding it about "real time guarantees." This may be difficult to quantify exactly. It does seem
reasonable to propose that a radically increased execution speed may lead to a language implementation that can keep
more realtime promises; if the language is running faster there's more free CPU time to execute other scheduled tasks,
meaning the tasks spend less time waiting in the queue once they are ready to execute.

It makes sense to try and quantify the current implementation's ability to deliver on timing requirements while under
load. One idea is to run task on some regular interval, say 100ms, to send an OSC message to a data collection program
running in a different process. Then the sclang program could run programs of increasing synthetic compute and memory
load, such as computing Fibonacci numbers. The external program could track the arrival time of the periodic message and
measure the jitter as a function of load.

Execution speed is important in SuperCollider but primarily due to its second-order effects, such as likely increase in
the stability of the real-time processes under load, the ability for more of the libraries and language to be
implemented in Hadron instead of in compiled languages linked in as binary blobs, and extending the reach of Hadron into
less expensive microcomputer and embedded contexts, such as EuroRack modules or wearables.

## Compilation Speed

Because Hadron JITs to native bytecode, including a few rounds of optimization, there may be scenarios where compilation
speed is significantly slower than the current implementation. There should be options to influence compilation to allow
for tradeoffs between compilation speed and optimization.

### Threading and Concurrency

## Stability

## Modularity

Hadron should be strongly modular, leveraging its distribution mechanism (Quarks) and lazy dynamic library loading to a
large degree. This will allow for many dependencies to be maintained and updated independently from the core language,
and allows us to hold the core library to a very high software engineering standard without gatekeeping distribution of
other modules that, while popular, may not meet that standard.

### Core Components

Hadron's modularity and speed encourages keeping the majority of code written in SC language and in separate modules.
But some things are fundamental to the language and should be destributed in the core. Here is a list of those items:

 * Primitive data types: `int`, `char`, `string`, `symbol`, `float`, `bool`
 * Fundamental data structures: `Array`, `List`, `Dictionary`, `IdentityDictionary`
 * Timing essentials: `Clock`, `SystemClock`, etc
 * Testing and validation: `UnitTest`
 * Threading and synchronization: `Routine`, `Task`, etc
 * Interpreter and Compiler: `Interpreter`
 * The mechanics of `SynthDef` compilation, including `AbstractFunction`
 * Quark or similar system for code distribution

### Dynamic Linking and Library Calling

Some of the current sclang maintenence burden comes from the distribution and maintenence of binary dependencies such
as Qt. The static linking also prevents runtime configuration of things like X11 usage, complicating scripting in
server or embedded environments where such capabilities may not be present.

Calling C or C++-based library code directly from Hadron code will hopefully allow minimization of the build
configuration flags, as it could be possible to distribute smaller Hadron binaries with *runtime* (as opposed to
*buildtime*) dependencies.

### Homoiconicity

SCv3 has some options to influence the compilation of code, such as the `Interpreter.preprocessor` method. Hadron
extends this to add full metaprogramming capability, enabling access to and manipulation of the abstract syntax tree of
an expression in the language.

Important also to try and ensure that higher-level languages that rely on sclang as a backend interpreter, such as
Tidal, could continue to work (with modification) on Hadron.

### Code Distribution

The Quark system has been great for distributing SuperCollider code. We should keep it, possibly extending it as needed.
Tracking dependencies between modules needs improvement.

## Backwards Compatibility

Hadron will make a best effort to maintain backwards compatability with sclang, to allow for most code that ran on
sclang to also run on Hadron. There are some possibility for breaking changes to occur but to maximize tech transfer we
will keep these to a minimum.

## Developer Interface

### Error Messages

As SC is often the first and only programming language most users will learn, we will benefit from very clear error
messages. Static types could allow for stronger program analysis at compile time, allowing for fewer runtime errors and
better confidence in program correctness.

It would be also good to coordinate volunteers for translation of error messages to user languages other than English.

### Language Server Protocol / Debugger Adapter Protocol

Support for these protocols will allow for easier integration into editors like vscode.

## Documentation Support

There have been some interesting discussions about an overhaul to the schelp system to allow code and documentation to
live in the same file, doxygen style.

## Compatibility and Reach

32 and 64 bit builds, JIT bytecode generation for embedded, desktop, and server formats. Linux, Windows, MacOS X.

## Integrated Testing Support

UnitTest integration, profiler support adds built-in feedback about UnitTest coverage.

## Integrated Profiler and Debugger

Runtimes for code segments, line-by-line execution counts for things like code coverage, interactive debugger.

## Crash Reporting

Core dumps can be uploaded using a similar system to Scintillator for aggregation and analysis.