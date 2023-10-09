use std::io::Write;
use std::fmt;

use super::DiagnosticKind;

pub enum DiagnosticLevel {
    Note,
    Warning,
    Error,
}

/// A location in code referred to by the diagnostic.
/// TODO: does this need to be two lifetimes?
pub struct DiagnosticLocation<'code, 'name> {
    pub file_name: &'name str,
    pub line_number: u32,
    pub column_number: u32,

    pub line: &'code str,
}

impl<'code, 'name> fmt::Display for DiagnosticLocation<'code, 'name> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.file_name)?;
        if self.line_number > 0 {
            f.write_fmt(format_args!(":{}", self.line_number))?;
        }
        if self.column_number > 0 {
            f.write_fmt(format_args!(":{}", self.column_number))?;
        }
        fmt::Result::Ok(())
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

pub trait DiagnosticMessage<'c, 'n>: fmt::Display {
    fn kind(&self) -> DiagnosticKind;
    fn location(&self) -> &DiagnosticLocation<'c, 'n>;
}

// ***** An attempt to write what should be macro-generated code.
pub struct ParseUnknownTokenMessage<'c, 'n> {
    location: DiagnosticLocation<'c, 'n>,

    // extra state variables
    unknown: char,
}

impl<'c, 'n> DiagnosticMessage<'c, 'n> for ParseUnknownTokenMessage<'c, 'n> {
    fn kind(&self) -> DiagnosticKind { DiagnosticKind::ParseUnknownToken }
    fn location(&self) -> &DiagnosticLocation<'c, 'n> { &self.location }
}

impl<'c, 'n> fmt::Display for ParseUnknownTokenMessage<'c, 'n> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_fmt(format_args!("Unrecognized character sequence starting with '{}'",
            self.unknown))
    }
}
// ***** End of attempt to write what should be macro-generated.

/// A complete Diagnostic, including a main message and optional notes, plus the level.
pub struct Diagnostic<'c, 'n> {
    pub level: DiagnosticLevel,
    pub message: Box<dyn DiagnosticMessage<'c, 'n>>,
    pub notes: Vec<Box<dyn DiagnosticMessage<'c, 'n>>>,
}

/// An interface for an object that can receive diagnostics from the toolchain as they are emitted.
pub trait DiagnosticConsumer {
    fn handle_diagnostic(diag: Diagnostic);
    fn flush();
}

pub trait DiagnosticLocationTranslator<'c, 'n, LocationT> {
    fn get_location(loc: LocationT) -> DiagnosticLocation<'c, 'n>;
}

/* 
/// A helper structure for building a single diagnostic with a fluid API
pub struct DiagnosticBuilder<LocationT> {
}

impl<LocationT> DiagnosticBuilder<LocationT> {

}

// This is an adaptor between subsystems (like the lexer/parser) and the diagnostic system.
pub struct DiagnosticEmitter<LocationT> {
}

impl<LocationT> DiagnosticEmitter<LocationT> {
    pub fn emit(location: LocationT, diagnostic: DiagnosticBase, ...) -> DiagnosticBuilder<LocationT> {
    }

}
*/

pub struct StreamDiagnosticConsumer<'a> {
    stream: &'a dyn std::io::Write,
}

impl<'a> StreamDiagnosticConsumer<'a> {
    pub fn new(stream: &'a impl std::io::Write) -> StreamDiagnosticConsumer {
        StreamDiagnosticConsumer { stream }
    }
}

impl<'a> DiagnosticConsumer for StreamDiagnosticConsumer<'a> {
    fn handle_diagnostic(diag: Diagnostic) {
    }
    fn flush() {

    }
}
