# Code Coverage

Hadron aspires for 100% code coverage with tests, meaning that every line of production Hadron
code is exercised at least once by a unit or integration test. Robust unit test coverage adds many
benefits to a project, but one important one is that it gives newcomers to the project an added 
sense of security, because contributors and reviewers both get timely and accurate feedback about
the correctness of their changes.

To further our code coverage goal we are tracking code coverage using
[codecov.io](https://app.codecov.io/gh/hadron-sclang/hadron). Any PR uploaded to GitHub should have
automated code coverage information reported by the codecov bot. It's also possible to compute that
same coverage locally, for consumption in either the diverse tooling available for code coverage
visualization or via an automatically generated HTML report.

This guide explains how to install the prerequisite tooling and how to generate the reports, and
provides some recommendations about their consumption.

## Setup

Mozilla provides `grcov`, which is a useful tool for organizing rust coverage data into a format
consumable by the rest of the coverage pipeline. You can install it via cargo:

```
cargo install grcov
```

In addition you will need `llvm-profdata`, if you don't have it already from an LLVM system
install, `rustup` can provide it:

```
rustup component add llvm-tools-preview
```

## Collecting Coverage Reports

We have automated the standard coverage collection process and included it in cargo using the 
`xtask` crate. To collect coverage data run:

```
cargo xtask cov
```

The `cov` task supports a single optional flag argument `--report`, which when provided will
generate an HTML report accessible at `target/coverage/html/index.html`. If the `--report` flag is
not provided `cov` will produce a machine-readable coverage report as
`target/coverage/tests.lcov`, which can be consumed by a variety of coverage tooling, and is
what the codecov bot uses to track coverage changes in a PR.

## Visual Studio Code Extension - Coverage Gutters

There are a few VSCode extensions that can display code coverage. Some recommend "Coverage
Gutters," which can show red/green coverage information on the left-hand side of the text editor,
next to the line numbers. Coverage Gutters must be configured to find the coverage info by adding
"tests.lcov" to the list of Coverage file names.

If you've found another tool you like for code coverage, please suggest it here with a PR!
