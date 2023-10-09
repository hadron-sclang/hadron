//! Tools for the production and management of diagnostic feedback from the toolchain.
//!
//! This design follows the Carbon toolchain design.

pub mod diagnostic_emitter;
pub mod diagnostic_kind;

//pub use diagnostic_emitter::DiagnosticEmitter;
pub use diagnostic_kind::DiagnosticKind;
