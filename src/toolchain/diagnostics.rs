//! Tools for the production and management of diagnostic feedback from the toolchain.
//!
//! This design follows the Carbon toolchain design somewhat.

pub enum DiagnosticLevel {
    Note,
    Warning,
    Error,
}

/// The enumerated type of all diagnostics Hadron emits.
pub enum DiagnosticKind {
    ParseError,
}

/// A location in code referred to by the diagnostic.
pub struct DiagnosticLocation<'a> {
    pub file_name: &'a str,
    pub line: &'a str,
    pub line_number: u32,
    pub column_number: u32,
}

/// A single diagnostic message, part of a larger Diagnostic. Includes its own formatting
/// information.
pub struct DiagnosticMessage<'a> {
    pub kind: DiagnosticKind,
    pub location: DiagnosticLocation<'a>,
    pub format: &str,
}

/// A complete Diagnostic, including a main message and optional notes, plus the level.
pub struct Diagnostic<'a> {
    pub level: DiagnosticLevel,
    pub message: DiagnosticMessage<'a>,
    pub notes: std::Vec<DiagnosticMessage<'a>>,
}

/// An interface for an object that can receive diagnostics from the toolchain as they are emitted.
pub trait DiagnosticConsumer {
    pub fn HandleDiagnostic(diag: Diagnostic);
    pub fn Flush();
}

/// A helper structure for building a single diagnostic with a fluid API
pub struct DiagnosticBuilder<LocationT> {
}

pub impl DiagnosticBuilder<LocationT> {

}

// This is an adaptor between subsystems (like the lexer/parser) and the diagnostic system.
pub struct DiagnosticEmitter<LocationT> {
}

pub impl DiagnosticEmitter<LocationT> {
    pub fn Emit(location: LocationT, diagnostic: DiagnosticBase, ...) -> DiagnosticBuilder<LocationT> {
    }

}


pub struct StreamDiagnosticConsumer {

}
