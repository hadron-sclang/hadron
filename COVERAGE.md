# Code Coverage

Hadron aspires for 100% code coverage with unit tests, meaning that every line of production Hadron
code is exercised at least once by a unit or integration test. Robust unit test coverage adds many
benefits to a project, but one important one is that it gives relative newcomers to the project an
added sense of security, contributors and reviewers both get ample feedback about the correctness
of their changes.



## Setup

Mozilla provides `grcov`, which is a useful tool for organizing rust coverage data into a format
consumable by the rest of the LLVM coverage pipeline

```
cargo install grcov
```

You will need `llvm-profdata`, if you don't have it already from an LLVM system install, `rustup` 
can provide it:

```
rustup component add llvm-tools-preview
```