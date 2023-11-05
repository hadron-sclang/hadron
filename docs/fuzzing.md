# Fuzzing

People use SuperCollider language in live performance settings, so it is important that a
SuperCollider interpreter never crash or hang, even with invalid or corrupted input. *Fuzzing* is
a useful technique to catch these kinds of bugs.

Fuzzing is the process of applying random inputs to the program under test, in this case Hadron,
to try to find specific inputs that cause crashes or hangs in the program. Modern fuzzing uses
*instrumented builds* of the program under test, attempting to find interesting input sequences
that cause more of the program under test to execute than previous sequences did.

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

Hadron does not normally use the Rust nightly toolchain to build, so for non-fuzzing workflows we
compile and test Hadron with the standard toolchain. A developer can configure `rustup` to use
either toolchain as the default, but on the balance it may be easier to keep the standard toolchain
as the default and specify to use the nightly toolchain to cargo whenever using `cargo-fuzz`. This
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
and load new data while fuzzing. To avoid bloat (and possible viral license implications), and
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
compilation phases allows for deeper testing of more of the compiler.

### Decide On Parallelism

As fuzzing is a randomized search, it's also important to run as many iterations of fuzzing as
possible, in order to maximize the odds of finding novel bugs. Best practice is to use the same
number of fuzzing jobs as computer cores on the machine, although bear in mind that fuzzing jobs
can last for *weeks* which during they will consume the *entire capacity* of that CPU core, so bear
in mind the usability and machine health considerations when determining the level of parallelism
to apply.

On Linux, the `lscpu` command will report the number of cores on the machine. I typically choose to
fuzz at that number of jobs minus 1 or 2, to leave some room for the computer to service other
processes, albiet slowly.

### Consider Fuzzing Release Builds

By default the fuzzer uses development builds, which come with more checks and safeguards but are
also slower. It's useful to sometimes fuzz release builds, as these are faster so you get more fuzz
cycles, in addition to more accurately reflecting productions builds. To fuzz release builds, add 
a `--release` flag to the command as documented [below](#fuzz).

### Fuzz!

With target and number of jobs selected, invoke the fuzzing command as:

```
~/src/hadron$ cargo +nightly fuzz run <target> --jobs=<number of jobs>
```

Or if you've chosen to fuzz in release:

```
~/src/hadron$ cargo +nightly fuzz run --release lex
```

And then let it run. If the fuzzer determines it has exhausted its search it will stop on its own, 
otherwise you can stop fuzzing at any time by hitting CTRL+C.

### Findings

Findings are reported by `cargo-fuzz` with a big blob of text, starting with a backtrace:

```
=================================================================
==30497==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000001532 at pc 0x55f6613042c9 bp 0x7ffd1710cc50 sp 0x7ffd1710cc48
READ of size 1 at 0x502000001532 thread T0
    #0 0x55f6613042c8 in core::str::validations::next_code_point::h3bc4ddd5e834202d /rustc/9d83ac217957eece2189eccf4a7232caec7232ee/library/core/src/str/validations.rs:56:26
    #1 0x55f6613042c8 in _$LT$core..str..iter..Chars$u20$as$u20$core..iter..traits..iterator..Iterator$GT$::next::h46f1ce2c2c5b7412 /rustc/9d83ac217957eece2189eccf4a7232caec7232ee/library/core/src/str/iter.rs:44:18
    #2 0x55f6613042c8 in hadron_sclang::toolchain::lexer::cursor::Cursor::bump::h4164eefd819799df /home/luci/src/hadron/src/toolchain/lexer/cursor.rs:174:31
    #3 0x55f661301a79 in _$LT$hadron_sclang..toolchain..lexer..cursor..Cursor$u20$as$u20$core..iter..traits..iterator..Iterator$GT$::next::h4036875bb67c7590 /home/luci/src/hadron/src/toolchain/lexer/cursor.rs:36:32
    #4 0x55f6612ecb6d in _$LT$alloc..vec..Vec$LT$T$GT$$u20$as$u20$alloc..vec..spec_from_iter_nested..SpecFromIterNested$LT$T$C$I$GT$$GT$::from_iter::h4f7ee0f05a688378 /rustc/9d83ac217957eece2189eccf4a7232caec7232ee/library/alloc/src/vec/spec_from_iter_nested.rs:26:32
    #5 0x55f661300a30 in _$LT$alloc..vec..Vec$LT$T$GT$$u20$as$u20$alloc..vec..spec_from_iter..SpecFromIter$LT$T$C$I$GT$$GT$::from_iter::h5efa471e3d86d9b7 /rustc/9d83ac217957eece2189eccf4a7232caec7232ee/library/alloc/src/vec/spec_from_iter.rs:33:9
    #6 0x55f661300a30 in _$LT$alloc..vec..Vec$LT$T$GT$$u20$as$u20$core..iter..traits..collect..FromIterator$LT$T$GT$$GT$::from_iter::he771c91f5490a9ca /rustc/9d83ac217957eece2189eccf4a7232caec7232ee/library/alloc/src/vec/mod.rs:2753:9
    #7 0x55f661300a30 in core::iter::traits::iterator::Iterator::collect::he76a5bfd907dfa41 /rustc/9d83ac217957eece2189eccf4a7232caec7232ee/library/core/src/iter/traits/iterator.rs:2054:9
    #8 0x55f661300a30 in hadron_sclang::toolchain::lexer::tokenized_buffer::TokenizedBuffer::tokenize::ha688deb5b5b68718 /home/luci/src/hadron/src/toolchain/lexer/tokenized_buffer.rs:20:29
    #9 0x55f6612ec47f in lex::_::__libfuzzer_sys_run::hcc43e1ef8930e81f /home/luci/src/hadron/fuzz/fuzz_targets/lex.rs:9:13

... skipping a bunch of cargo-fuzz stack frames here ...

    #25 0x55f66120bd44 in _start (/home/luci/src/hadron/fuzz/target/x86_64-unknown-linux-gnu/release/lex+0x27fd44) (BuildId: 748a73b2bb200913b71ecb568ec538e89c6cdfcf)

0x502000001532 is located 0 bytes after 2-byte region [0x502000001530,0x502000001532)
allocated by thread T0 here:
    #0 0x55f6612a681e in malloc /rustc/llvm/src/llvm-project/compiler-rt/lib/asan/asan_malloc_linux.cpp:69:3
    #1 0x7fd3d18ae98b in operator new(unsigned long) (/lib/x86_64-linux-gnu/libstdc++.so.6+0xae98b) (BuildId: e37fe1a879783838de78cbc8c80621fa685d58a2)
    #2 0x55f6616a4e53 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) /home/luci/.cargo/registry/src/index.crates.io-6f17d22bba15001f/libfuzzer-sys-0.4.7/libfuzzer/FuzzerLoop.cpp:514:22
    #3 0x55f6616a5e48 in fuzzer::Fuzzer::MutateAndTestOne() /home/luci/.cargo/registry/src/index.crates.io-6f17d22bba15001f/libfuzzer-sys-0.4.7/libfuzzer/FuzzerLoop.cpp:758:25
    #4 0x55f6616a8237 in fuzzer::Fuzzer::Loop(std::vector<fuzzer::SizedFile, std::allocator<fuzzer::SizedFile> >&) /home/luci/.cargo/registry/src/index.crates.io-6f17d22bba15001f/libfuzzer-sys-0.4.7/libfuzzer/FuzzerLoop.cpp:903:21
    #5 0x55f6616bc64f in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) /home/luci/.cargo/registry/src/index.crates.io-6f17d22bba15001f/libfuzzer-sys-0.4.7/libfuzzer/FuzzerDriver.cpp:912:10
    #6 0x55f66120bba6 in main /home/luci/.cargo/registry/src/index.crates.io-6f17d22bba15001f/libfuzzer-sys-0.4.7/libfuzzer/FuzzerMain.cpp:20:30
    #7 0x7fd3d1429d8f  (/lib/x86_64-linux-gnu/libc.so.6+0x29d8f) (BuildId: 229b7dc509053fe4df5e29e8629911f0c3bc66dd)

SUMMARY: AddressSanitizer: heap-buffer-overflow /rustc/9d83ac217957eece2189eccf4a7232caec7232ee/library/core/src/str/validations.rs:56:26 in core::str::validations::next_code_point::h3bc4ddd5e834202d
Shadow bytes around the buggy address:
  0x502000001280: fa fa 00 00 fa fa fd fa fa fa fd fa fa fa fd fa
  0x502000001300: fa fa fd fd fa fa fd fd fa fa fd fa fa fa fd fa
  0x502000001380: fa fa fd fd fa fa fd fa fa fa fd fa fa fa fd fa
  0x502000001400: fa fa fd fa fa fa fd fa fa fa 03 fa fa fa 00 fa
  0x502000001480: fa fa fd fa fa fa fd fa fa fa fd fa fa fa fd fd
=>0x502000001500: fa fa fd fd fa fa[02]fa fa fa fa fa fa fa fa fa
  0x502000001580: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000001600: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000001680: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000001700: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  0x502000001780: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Partially addressable: 01 02 03 04 05 06 07 
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack left redzone:      f1
  Stack mid redzone:       f2
  Stack right redzone:     f3
  Stack after return:      f5
  Stack use after scope:   f8
  Global redzone:          f9
  Global init order:       f6
  Poisoned by user:        f7
  Container overflow:      fc
  Array cookie:            ac
  Intra object redzone:    bb
  ASan internal:           fe
  Left alloca redzone:     ca
  Right alloca redzone:    cb
==30497==ABORTING
MS: 1 InsertByte-; base unit: 067d5096f219c64b53bb1c7d5e3754285b565a47
0xe6,0xb,
\346\013
artifact_prefix='/home/luci/src/hadron/fuzz/artifacts/lex/'; Test unit written to /home/luci/src/hadron/fuzz/artifacts/lex/crash-aebb5cb819bee464ae8562d6f15645fab35bb87b
Base64: 5gs=

────────────────────────────────────────────────────────────────────────────────

Failing input:

	fuzz/artifacts/lex/crash-aebb5cb819bee464ae8562d6f15645fab35bb87b

Output of `std::fmt::Debug`:

	[230, 11]

Reproduce with:

	cargo fuzz run -O lex fuzz/artifacts/lex/crash-aebb5cb819bee464ae8562d6f15645fab35bb87b

Minimize test case with:

	cargo fuzz tmin -O lex fuzz/artifacts/lex/crash-aebb5cb819bee464ae8562d6f15645fab35bb87b

────────────────────────────────────────────────────────────────────────────────

Error: Fuzz target exited with exit status: 1

```

If the backtrace doesn't have symbols but rather only addresses, looking like:

```
==29007==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000251 at pc 0x55d934f737e8 bp 0x7ffebe467f30 sp 0x7ffebe467f28
READ of size 1 at 0x502000000251 thread T0
    #0 0x55d934f737e7  (/home/luci/src/hadron/fuzz/target/x86_64-unknown-linux-gnu/release/lex+0x37a7e7) (BuildId: d6fd78f1b3c74e07b254358f05387df58b49b38c)
    #1 0x55d934f70e0f  (/home/luci/src/hadron/fuzz/target/x86_64-unknown-linux-gnu/release/lex+0x377e0f) (BuildId: d6fd78f1b3c74e07b254358f05387df58b49b38c)
    #2 0x55d934f5be7d  (/home/luci/src/hadron/fuzz/target/x86_64-unknown-linux-gnu/release/lex+0x362e7d) (BuildId: d6fd78f1b3c74e07b254358f05387df58b49b38c)

<snip>
```

You will need to ensure `llvm-symbolizer` is somewhere in your `$PATH`. On Ubuntu systems this is
easily solved by installing LLVM:

```
sudo apt-get install llvm
```

**Note:** `cargo-fuzz` saves the input that caused the crash to the `fuzz/artifacts/<target>/`
directory, and includes instructions on how to reproduce the crash. The `fuzz/artifacts` directory
is also ignored by `git`, so as to not clutter up the repository with gigabytes of possibly
redundant tiny files. That said, these findings are very useful, if noisy. Getting a fuzzer report
means you found a Hadron bug! Thank you! Please report it!

### Reporting Fuzzer Bugs

We organize fuzzer artifacts by *rust* stack trace. In the [example finding](#findings) above, it
looks like the crash happens in:

```
0x55f6613042c8 in hadron_sclang::toolchain::lexer::cursor::Cursor::bump::h4164eefd819799df
/home/luci/src/hadron/src/toolchain/lexer/cursor.rs:174:31
```

or the `toolchain::lexer::cursor::Cursor::bump` method.

To report this finding, first see if there are any existing
[fuzzer-findings issue
reports](https://github.com/hadron-sclang/hadron/issues?q=is%3Aopen+is%3Aissue+label%3Afuzzer-findings)
with similar stack traces. If you can't tell for certain, please feel free to file the issue
anyway, it's far better to receive duplicate issue reports than to miss an important one.

Select the "Fuzzer Findings Bug" template from the list
[here](https://github.com/hadron-sclang/hadron/issues/new/choose) and file the bug.

The template asks for the `cargo-fuzz` target, for a pasted copy of the fuzzer finding, and for the
artifact file as an attachment. Note that you will have to put the attachment in a zip file for 
GitHub to accept the attachment.

For an example fuzzer-findings bug see [#142](https://github.com/hadron-sclang/hadron/issues/142). 

### Fixing Fuzzer Bugs

For the most part, the process of fixing fuzzer bugs is similar to fixing any other "reproduceable
crash/hang" issue. The difficulty of these bugs is somewhat variable, but in general they may be
easier to fix because of the reproduceability.

Fuzzer bugfix commits should include the reproducer input file. Please add it to the
target-specific subdirectory in `/tests/fuzz`. 

To simplify debugging of fuzzer crashes it is sometimes easier to use the `scc` binary provided
by Hadron, which has flags to take compilation to each stage before stopping, and can usually
reproduce the crash on the example input without all of the fuzzing infrastructure in the way.
