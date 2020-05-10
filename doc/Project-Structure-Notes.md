Project Structure Notes
=======================

There should be a separate hadron binary that has its own command-line arguments and can serve as a distinctive
standalone sc interpreter. Then there's a separate executable which is called sclang and follows the command-line
behavior of current sclang. It may also automatically include certain modules that are out-of-scope for Hadron.

src/ contains the source for Hadron. The interpreter itself is a library with an API called by both hadron and sclang.
Also need 100% unit test coverage for libhadron.

bench/ also uses the hadron library and contains a series of benchmark tests. It is compiled separately from the
language. It has a standalone benchmark suite that links against libhadron and generates a human-readable (or JSON blob)
report of different algorithms within libhadron. This will plug into the perf farm and get harvested into the overall
perf story. These benchmarks are about keeping hadron as fast as possible, are whitebox, and aren't about comparisons to
LSC (legacy sclang).

Black-box comparisons only for now against traditional sclang. Apples-to-apples comparisons on a variety of different
workloads. Timing response from sending UDP OSC message to language process and receiving correct "done" response also
via UDP. Averaged over many times looking at stasitical significance.

test/ contains integration test suite for hadron. This is about correctness.