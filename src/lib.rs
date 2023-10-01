//! SuperCollider language compiler, runtime, and toolchain.
//!
//! Hadron is a re-implementation of the
//! [SuperCollider](https://supercollider.github.io/) language interpreter. This crate contains
//! tools for SuperCollider language analysis, compilation, and execution.
//!

#[macro_use]
extern crate static_assertions;

pub mod runtime;
pub mod toolchain;
