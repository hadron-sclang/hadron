//! Tools for the production and management of diagnostic feedback from the toolchain.
//!
//! This design loosely follows the Carbon toolchain design. The goal is to allow other parts of
//! the toolchain to emit diagnostic messages as structured types supporting the
//! [DiagnosticMessage] trait.
//!
//! Toolchain code will normally construct a [Diagnostic] by using a [DiagnosticEmitter] that
//! translates domain-specific locations (for example a [Token] for the lexer and parser) with
//! the use of a [DiagnosticLocationTranslator] and sends it to a [DiagnosticConsumer], which
//! delivers the diagnostic messages to the user.
//!

pub mod diagnostic_emitter;
pub mod diagnostic_kind;

pub use diagnostic_emitter::DiagnosticEmitter;
pub use diagnostic_emitter::DiagnosticLocation;
pub use diagnostic_emitter::DiagnosticLocationTranslator;
pub use diagnostic_kind::DiagnosticKind;
