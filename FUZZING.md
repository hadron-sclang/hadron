# Fuzzing

People use SuperCollider language in live performance settings, so it is important that a
SuperCollider interpreter never crash or hang, even with invalid or corrupted input. *Fuzzing* is
a useful technique to catch these kinds of bugs.

Fuzzing is the process of applying random inputs to the program under test, in this case Hadron,
to try to find specific inputs that cause crashes or hangs in the program. Modern fuzzing uses
*instrumented builds* of the program under test, attempting to find interesting input sequences
that cause more of the program under test to execute than their siblings.

**Note:** Fuzzing is *compute intensive*, and in a well-fuzzed application it may take *hours,
days, or weeks* of fuzzing to find a novel bug. During that time the computer will work at maximum
CPU utilization, so be advised that this can be quite demanding of computer hardware, particularly
thermally-constrained platforms like laptops.

# Fuzzing Hadron With `cargo-fuzz`

Since **0.1.0-alpha-3** Hadron supports fuzzing with `cargo-fuzz`
([git](https://github.com/rust-fuzz/cargo-fuzz),
[doc](https://rust-fuzz.github.io/book/cargo-fuzz.html)), a popular rust crate for fuzzing
rust programs. Here we document how to setup a computer for fuzzing Hadron, and what to do when
your fuzzing finds issues in the code.

## Setup

**Note:** These instructions are currently tested only on Linux. While Hadron itself has strong
goals of multiplatform support, the underlying fuzzing infrastructure is most thoroughly tested on
Linux. Your mileage may vary on other operating systems. If you have success on other platforms,
please update this documentation with what you've learned!

### Prerequisites

The `cargo-fuzz` crate relies on the rust nightly toolchain, so this needs to be installed before
installing the fuzzing tools. If you installed rust using `rustup`, it can install the nightly 
toolchain with this shell command:

```
$ rustup install nightly
```

Hadron does not use the Rust nightly toolchain to build, so for non-fuzzing workflows we compile
and test Hadron with the standard toolchain. A developer can configure `rustup` to use either
toolchain as the default, but on the balance it may be easier to keep the standard toolchain as the
default and specify to use the nightly toolchain to cargo whenever using `cargo-fuzz`. This
documentation follows that convention, so will include the `+nightly` argument in each cargo
fuzzing command.

### Install `cargo-fuzz`

Then, in the hadron source root directory, in this example in `~/src/hadron`, install the
`cargo-fuzz` crate:

```
~/src/hadron$ cargo install cargo-fuzz
```

You should now be ready to fuzz! Test the successful install of `cargo-fuzz` by asking it to list
available fuzzing targets:

```
~/src/hadron$ cargo +nightly fuzz list
```

Which should respond with a list of the currently avaialable targets for fuzzing.

## `cargo-fuzz` Fuzzing Workflow

### Optional But Recommended: Provide Example Inputs

The fuzzer can do an adequate job on its own trying to find "interesting" inputs to Hadron that
can cause more of the program code to execute, but it is greatly aided by providing some examples
of inputs that it can work from.

`cargo-fuzz` is configured to scan the `fuzz/corpus` directoy for example input and can even detect
and load new data while fuzzing. To avoid bloat (and possible viral license transfer), as well as
to help keep some diversity in example inputs between people fuzzing Hadron, `git` will ignore any
files added to this directory. So, feel free to copy lots of example SuperCollider code here,
inluding Quarks, class library code, scripts you've written, even broken or incomplete code is
useful. The more data in here, the richer the library `cargo-fuzz` has to draw from to compose
novel inputs.

### Pick A Fuzzing Target

First, select a target to fuzz:

```
~/src/hadron$ cargo +nightly fuzz list
```

Some targets are inclusive of others, for example fuzzing the parser will necessarily require
lexing the input first, so the lexer also receives fuzzing. We maintain the lexer as an independent
target because fuzzing can be more productive on smaller programs, allowing it to fit more
iterations into the same unit of time. Furthermore, the lexer in particular is the "first line of
defense" from invalid input, so it deserves the extra attention. On the other hand, choosing later
compilation phases allows for deeper testing of the later compilation stages.

It might be useful to think of early compilation stages searching for bugs in more breadth, as they
can run faster and therefore allow for more trials, and later compilation stages search in more
depth, as they execute more of the compiler.

### Decide On Parallelism

As fuzzing is a randomized search, it's also important to run as many iterations of fuzzing as
possible, in order to maximize the odds of finding novel bugs. Best practice is to use the same
number of fuzzing jobs as computer cores on the machine, although bear in mind that fuzzing jobs
can last for *weeks or months* which during they will consume the *entire capacity* of that CPU
core, so bear in mind the usability and machine health considerations when determining the level of
parallelism to apply.

On Linux, the `lscpu` command will report the number of cores on the machine. I typically choose to
fuzz at that number minus 1 or 2, to leave some room for the computer to service other processes,
albiet slowly.

### Fuzz!

With target and number of jobs selected, invoke the fuzzing command as:

```
~/src/hadron$ cargo +nightly fuzz run <target> --jobs=<number of jobs>
```

And then let it run. If the fuzzer finds a great many bugs it will stop on its own, otherwise you
can stop fuzzing at any time by hitting CTRL+C.

### Findings

Findings are reported by `cargo-fuzz` as:
```
────────────────────────────────────────────────────────────────────────────────

Failing input:

	fuzz/artifacts/lex/crash-54d9cd225843c137bdc9d294927fac104bea61dc

Output of `std::fmt::Debug`:

	[20, 252]

Reproduce with:

	cargo fuzz run lex fuzz/artifacts/lex/crash-54d9cd225843c137bdc9d294927fac104bea61dc

Minimize test case with:

	cargo fuzz tmin lex fuzz/artifacts/lex/crash-54d9cd225843c137bdc9d294927fac104bea61dc

────────────────────────────────────────────────────────────────────────────────
```

Note that `cargo-fuzz` saves the input that caused the crash to the `fuzz/artifacts/<target>/`
directory. The `fuzz/artifacts` directory is also ignored by `git`, so as to not clutter up the
repository with gigabytes of possibly redundant tiny files. That said, these findings are very
useful, if noisy; you found a Hadron bug! Thank you!

### Reporting Fuzzer Bugs

We do keep some fuzzer artifacts in version control, they are in the `/tests/`

### Fixing Fuzzer Bugs

- Choosing an input

Fuzzers produce randomized input that is not neatly organized into per-bug categories. You may find
a fuzzer run producing a great many artifacts. cargo-fuzz configures the fuzz/artifacts/ directory
as ignored by git, so nothing generated there will make it into version control.

.. some more about fixing one at a time, fixing, then screening the rest in artifacts/ against
that fix

Minimize corpus, although this can get quite large..


Where to move chosen inputs?
```
mv fuzz/artifacts/tokenized_buffer/crash-080d417f4cdbbe27b5aa37b5487a8796567b9f4c \
    fuzz/exemplars/tokenized_buffer/.
```

- Fixing the bug

cargo-fuzz normally builds in release, also useful to run things in debug too, get more detailed
stack info
