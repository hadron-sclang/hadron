//! So much stuff to do:
//! [x] Figure out namespaces and crate structures
//! [x] TokenizedBuffer? Should Token become an index type, rename existing to TokenInfo?
//! [x] How to get `line` &str into the token?
//! [x] Diagnostics usability/macros/emission
//! [ ] Implement parsing state machine
//! [ ] Parsing state machine in other files (expr!)
//! [ ] Parser unit/smoke tests
//! [ ] Documentation
//! [ ] Integration testing?
//! [ ] After merge - fuzz testing
//! [ ] After merge - benchmarks for lexer and parser + "speed of light"
pub mod node;
pub mod tree;

mod context;

