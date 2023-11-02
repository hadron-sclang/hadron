rustup install nightly

Maybe?
sudo apt-get install libssl-dev

cargo install cargo-fuzz

cargo +nightly fuzz list

```
cargo +nightly fuzz run <target> --jobs=<number of jobs>
```

-- Feed me weird things
```
cargo +nightly fuzz cmin tokenized_buffer
```

Findings look like:
```
────────────────────────────────────────────────────────────────────────────────

Failing input:

	fuzz/artifacts/tokenized_buffer/crash-54d9cd225843c137bdc9d294927fac104bea61dc

Output of `std::fmt::Debug`:

	[20, 252]

Reproduce with:

	cargo fuzz run tokenized_buffer fuzz/artifacts/tokenized_buffer/crash-54d9cd225843c137bdc9d294927fac104bea61dc

Minimize test case with:

	cargo fuzz tmin tokenized_buffer fuzz/artifacts/tokenized_buffer/crash-54d9cd225843c137bdc9d294927fac104bea61dc

────────────────────────────────────────────────────────────────────────────────
```

-- Fixing Fuzzer Bugs

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
