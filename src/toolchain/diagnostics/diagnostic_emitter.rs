use std::io::Write;
use std::fmt;

use super::DiagnosticKind;

pub enum DiagnosticLevel {
    Note,
    Warning,
    Error,
}

/// A location in code referred to by the diagnostic.
pub struct DiagnosticLocation<'s> {
    pub file_name: &'s str,
    pub line_number: u32,
    pub column_number: u32,

    pub line: &'s str,
}

impl<'s> fmt::Display for DiagnosticLocation<'s> {
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
/// And also to separate out the message formatting work from the rest of the toolchain. Which,
/// like, yeah.
pub trait DiagnosticMessage<'s>: fmt::Display {
    fn kind(&self) -> DiagnosticKind;
    fn location(&self) -> &DiagnosticLocation<'s>;
}

macro_rules! diag_decl {
    ($e: ident, $m: literal, $($t:ty, $x:expr)* ) => {
        pub struct concat_idents!($e, Message<'s>) {
            location: DiagnosticLocation<'s>,
        }
    }
}

#[macro_export]
macro_rules! diag {
    ( $e:ident, $f:literal, $($t:ty, $x:expr)* ) => {
        diag_decl!($e, $m, $($t, $x,)*)

    }
}

// ***** An attempt to write what should be macro-generated code.
pub struct ParseUnknownTokenMessage<'s> {
    location: DiagnosticLocation<'s>,

    // extra state variables
    unknown: char,
}

impl<'s> DiagnosticMessage<'s> for ParseUnknownTokenMessage<'s> {
    fn kind(&self) -> DiagnosticKind { DiagnosticKind::ParseUnknownToken }
    fn location(&self) -> &DiagnosticLocation<'s> { &self.location }
}

impl fmt::Display for dyn DiagnosticMessage<'_> {
    fn fmt(&self, f: fmt::Formatter<'_>) -> fmt::Result {
        // This is an abuse of the "alternate" syntax in fmt::Display trait used to pass a boolean
        // argument to fmt(), in this case to tell the DiagnosticMessage to print this message
        // as an error.
        let infix = match f.alternate() {
            true => "ERROR: ",
            false => ""
        };
        f.fmt(format_args!("{}: {}{}", self.location(), infix, /* TODO */ infix))
    }
}

impl<'s> fmt::Display for ParseUnknownTokenMessage<'s> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {

        f.write_fmt(format_args!("Unrecognized character sequence starting with '{}'",
            self.unknown))
    }
}
// ***** End of attempt to write what should be macro-generated.

/// A complete Diagnostic, including a main message and optional notes, plus the level.
pub struct Diagnostic<'s> {
    pub level: DiagnosticLevel,
    pub message: Box<dyn DiagnosticMessage<'s>>,
    pub notes: Vec<Box<dyn DiagnosticMessage<'s>>>,
}

impl<'s> fmt::Display for Diagnostic<'s> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let prefix = match self.level {
            DiagnosticLevel::Error => "ERROR",
            _ => ""
        };
    }
}

/// An interface for an object that can receive diagnostics from the toolchain as they are emitted.
pub trait DiagnosticConsumer {
    fn handle_diagnostic(&self, diag: Diagnostic);
    fn flush(&self);
}

/* 
pub trait DiagnosticLocationTranslator<'s, LocationT> {
    fn get_location(&self, loc: LocationT) -> DiagnosticLocation<'s>;
}

/// A helper structure for building a single diagnostic with a fluid API
pub struct DiagnosticBuilder<LocationT> {
}

impl<LocationT> DiagnosticBuilder<LocationT> {
}


// This is an adaptor between subsystems (like the lexer/parser) and the diagnostic system.
pub struct DiagnosticEmitter<'s, LocationT> {
    consumer: Box<dyn DiagnosticConsumer>,
    translator: Box<dyn DiagnosticLocationTranslator<'s, LocationT>>,
    /* 
    build() -> &DiagnosticBuilder<LocationT>
    note() -> &DiagnosticBuilder<LocationT>
    emit()
    */
}

impl<'s, LocationT> DiagnosticEmitter<'s, LocationT> {
    pub fn build(location: LocationT, 
    pub fn emit(location: LocationT, diagnostic: Diagnostic<'s>) -> DiagnosticBuilder<LocationT> {
    }

}
*/
pub struct StreamDiagnosticConsumer<W: std::io::Write> {
    stream: std::io::BufWriter<W>
}

impl<W: std::io::Write> StreamDiagnosticConsumer<W> {
    pub fn new(stream: W) -> StreamDiagnosticConsumer<W> {
        StreamDiagnosticConsumer { stream: std::io::BufWriter::new(stream) }
    }
}

impl<W: std::io::Write> DiagnosticConsumer for StreamDiagnosticConsumer<W> {
    fn handle_diagnostic(&self, diag: Diagnostic) {
        self.stream.write_fmt(format_args!("{}", diag));
    }
    fn flush(&self) {
        self.stream.flush();
    }
}

pub fn console_diagnostic_consumer() -> StreamDiagnosticConsumer<impl std::io::Write> {
    StreamDiagnosticConsumer::new(std::io::stderr())
}