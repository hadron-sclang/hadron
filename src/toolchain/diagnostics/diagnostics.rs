//! Tools for the production and management of diagnostic feedback from the toolchain.
//!
//! This design follows the Carbon toolchain design.

use std::io::Write;
use std::fmt;

pub enum DiagnosticLevel {
    Note,
    Warning,
    Error,
}

/// The enumerated type of all diagnostics Hadron emits.
pub enum DiagnosticKind {
    ParseUnknownToken,
}

/// A location in code referred to by the diagnostic.
pub struct DiagnosticLocation<'a> {
    pub file_name: &str,
    pub line_number: u32,
    pub column_number: u32,

    pub line: &'a str,
}

impl<'a> fmt::Display for DiagnosticLocation<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.location.file_name)?;
        if self.location.line_number > 0 {
            f.write_fmt(format_args!(":{}", self.location.line_number))?;
        }
        if self.location.column_number > 0 {
            f.write_fmt(format_args!(":{}", self.location.column_number))?;
        }
        Result::ok()
    }
}

/// A single diagnostic message, part of a larger Diagnostic. Includes its own formatting
/// information.
/// We've made this a trait to allow for polymorphism in the message formatting. This is all to
/// avoid the cost of formatting strings in error messages unless its needed. Which, like, OK.
/*
pub struct DiagnosticMessage<'a> {
    pub kind: DiagnosticKind,
    pub location: DiagnosticLocation<'a>,
    pub format: &str,
    pub format_args: Vec<&dyn fmt::Display>,
}
*/

pub trait DiagnosticMessage<'a>: fmt::Display {
    pub fn kind() -> DiagnosticKind;
    pub fn location(&self) -> DiagnosticLocation<'a>;
}

// ***** An attempt to write what should be macro-generated code.
pub struct ParseUnknownTokenMessage<'a> {
    location: DiagnosticLocation<'a>,

    // extra state variables
    unknown: char,
}

pub impl DiagnosticMessage<'a> for ParseUnknownTokenMessage<'a> {
    fn kind() -> DiagnosticKind { DiagnosticKind::ParseUnknownToken }
    fn location(&self) -> DiagnosticLocation { location }
}

impl fmt::Display for ParseUnknownTokenMessage<'a> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_fmt(format_args!("Unrecognized character sequence starting with '{}'", unknown));
    }
}
// ***** End of attempt to write what should be macro-generated.

/// A complete Diagnostic, including a main message and optional notes, plus the level.
pub struct Diagnostic<'a> {
    pub level: DiagnosticLevel,
    pub message: &dyn DiagnosticMessage<'a>,
    pub notes: std::Vec<&dyn DiagnosticMessage<'a>>,
}

/// An interface for an object that can receive diagnostics from the toolchain as they are emitted.
pub trait DiagnosticConsumer {
    pub fn handle_diagnostic(diag: Diagnostic);
    pub fn flush();
}

pub trait DiagnosticLocationTranslator<LocationT> {
    pub fn get_location(loc: LocationT) -> DiagnosticLocation;
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
    pub fn emit(location: LocationT, diagnostic: DiagnosticBase, ...) -> DiagnosticBuilder<LocationT> {
    }

}

pub struct StreamDiagnosticConsumer {
    stream: &impl std::io::Write,
}

pub impl StreamDiagnosticConsumer {
    pub fn new(stream: &impl std::io::Write) -> StreamDiagnosticConsumer {
        StreamDiagnosticConsumer { stream }
    }

    fn print(message: &DiagnosticMessage) {
    }
}

pub impl DiagnosticConsumer for StreamDiagnosticConsumer {
    fn handle_diagnostic(diag: Diagnostic) {
        
    }
}
