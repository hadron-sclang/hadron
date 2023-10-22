use std::io::Write;
use std::fmt;

use super::DiagnosticKind;

#[derive(PartialEq)]
pub enum DiagnosticLevel {
    Note,
    Warning,
    Error,
}

/// A location in code referred to by the diagnostic.
pub struct DiagnosticLocation<'s> {
    pub file_name: &'s str,
    pub line_number: i32,
    pub column_number: i32,

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

pub struct DiagnosticMessage<'s> {
    pub kind: DiagnosticKind,
    pub location: DiagnosticLocation<'s>,
    pub body: String
}

impl<'s> fmt::Display for DiagnosticMessage<'s> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // This is an abuse of the "alternate" syntax in fmt::Display trait used to pass a boolean
        // argument to fmt(), in this case to tell the DiagnosticMessage to print this message
        // as an error.
        let infix = match f.alternate() {
            true => "ERROR: ",
            false => ""
        };
        f.write_fmt(format_args!("{}: {}{}", self.location, infix, self.body))
    }
}

/// A complete Diagnostic, including a main message and optional notes, plus the level.
pub struct Diagnostic<'s> {
    pub level: DiagnosticLevel,
    pub message: DiagnosticMessage<'s>,
    pub notes: Vec<DiagnosticMessage<'s>>,
}

impl<'s> Diagnostic<'s> {
    /// Builds a new diagnostic. Normally called by a [DiagnosticBuilder].
    pub fn new(level: DiagnosticLevel, message: DiagnosticMessage<'s>, notes: Vec<DiagnosticMessage<'s>>) -> Diagnostic<'s> {
        Diagnostic { level, message, notes }
    }
}

impl<'s> fmt::Display for Diagnostic<'s> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        if self.level == DiagnosticLevel::Error {
            f.write_fmt(format_args!("{:#}", self.message))?;
        } else {
            f.write_fmt(format_args!("{}", self.message))?;
        }
        for m in &self.notes {
            f.write_fmt(format_args!("{}", m))?;
        }
        fmt::Result::Ok(())
    }
}

/// An interface for an object that can receive diagnostics from the toolchain as they are emitted.
pub trait DiagnosticConsumer {
    fn handle_diagnostic(&mut self, diag: &Diagnostic);
    fn flush(&mut self);
}

pub trait DiagnosticLocationTranslator<'s, LocationT> {
    fn get_location(&self, loc: &LocationT) -> DiagnosticLocation<'s>;
}

// This is an adaptor between subsystems (like the lexer/parser) and the diagnostic consumer. It
// holds the conumer and translator and facilitates creating Diagnostics, and ultimately
// provides the completed diagnostics to the DiagnosticConsumer.
pub struct DiagnosticEmitter<'c, 's, LocationT> {
    consumer: &'c mut dyn DiagnosticConsumer,
    translator: &'s dyn DiagnosticLocationTranslator<'s, LocationT>,
}

impl<'c, 's, LocationT> DiagnosticEmitter<'c, 's, LocationT> {
    pub fn new(consumer: &'c mut dyn DiagnosticConsumer,
               translator: &'s dyn DiagnosticLocationTranslator<'s, LocationT>)
               -> DiagnosticEmitter<'c, 's, LocationT> {
        DiagnosticEmitter { consumer, translator }
    }

    pub fn build(&self, level: DiagnosticLevel, kind: DiagnosticKind, location: &LocationT, body: String)
                -> DiagnosticBuilder<'s, LocationT> {
        DiagnosticBuilder::build(level, kind, location, body, self.translator)
    }

    pub fn emit(&mut self, diagnostic: &Diagnostic) {
        self.consumer.handle_diagnostic(diagnostic);
    }
}

pub struct DiagnosticBuilder<'s, LocationT> {
    level: DiagnosticLevel,
    message: DiagnosticMessage<'s>,
    notes: Vec<DiagnosticMessage<'s>>,

    translator: &'s dyn DiagnosticLocationTranslator<'s, LocationT>,
}

impl<'s, LocationT> DiagnosticBuilder<'s, LocationT> {
    pub fn build(level: DiagnosticLevel, kind: DiagnosticKind, location: &LocationT, body: String,
            translator: &'s dyn DiagnosticLocationTranslator<'s, LocationT>)
        -> DiagnosticBuilder<'s, LocationT> {
        let loc = translator.get_location(location);
        let message = DiagnosticMessage { kind, location: loc, body };
        DiagnosticBuilder { level, message, notes: Vec::new(), translator }
    }

    pub fn note(&mut self, kind: DiagnosticKind, location: &LocationT, body: String)
        -> &DiagnosticBuilder<'s, LocationT> {
        let loc = self.translator.get_location(location);
        self.notes.push(DiagnosticMessage { kind, location: loc, body });
        self
    }

    pub fn emit(&self) -> Diagnostic<'s> {
        Diagnostic { level: self.level, message: self.message, notes: self.notes }
    }
}

pub struct StreamDiagnosticConsumer<W: std::io::Write> {
    stream: std::io::BufWriter<W>
}

impl<W: std::io::Write> StreamDiagnosticConsumer<W> {
    pub fn new(stream: W) -> StreamDiagnosticConsumer<W> {
        StreamDiagnosticConsumer { stream: std::io::BufWriter::new(stream) }
    }
}

impl<W: std::io::Write> DiagnosticConsumer for StreamDiagnosticConsumer<W> {
    fn handle_diagnostic(&mut self, diag: &Diagnostic) {
        self.stream.write_fmt(format_args!("{}", diag));
    }
    fn flush(&mut self) {
        self.stream.flush();
    }
}

pub fn console_diagnostic_consumer() -> StreamDiagnosticConsumer<impl std::io::Write> {
    StreamDiagnosticConsumer::new(std::io::stderr())
}