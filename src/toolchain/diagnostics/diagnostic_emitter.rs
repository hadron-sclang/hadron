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
    pub fn new(level: DiagnosticLevel, message: DiagnosticMessage<'s>) -> Diagnostic<'s> {
        Diagnostic { level, message, notes: Vec::new() }
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
/* TODO: bring this fluid-style API back. A Builder object is much better than the ugliness below.
/// A helper structure for building a single diagnostic with a fluid API. The emitter must outlive
/// it.
pub struct DiagnosticBuilder<'e, 's, 'c, LocationT> {
    diag: Diagnostic<'s>,
    emitter: &'e mut DiagnosticEmitter<'s, 'c, LocationT>,
}

impl<'e, 's, 'c, LocationT> DiagnosticBuilder<'e, 's, 'c, LocationT> {
    // Normally called by a DiagnosticEmitter, which provides the translator reference.
    pub fn build(emitter: &'e mut DiagnosticEmitter<'s, 'c, LocationT>,
                 level: DiagnosticLevel, kind: DiagnosticKind, location: &LocationT,
                 body: String) -> DiagnosticBuilder<'e, 's, 'c, LocationT> {
        // Translate location
        let loc = emitter.get_location(location);
        let message = DiagnosticMessage { kind, location: loc, body };
        let diag = Diagnostic::new(level, message);
        DiagnosticBuilder { diag, emitter }
    }

    pub fn note(&mut self, kind: DiagnosticKind, location: &LocationT, body: String)
                -> &DiagnosticBuilder<'e, 's, 'c, LocationT> {
        let loc = self.emitter.get_location(location);
        self.diag.notes.push(DiagnosticMessage { kind, location: loc, body });
        self
    }

    pub fn emit(&self) {
        self.emitter.emit(self.diag)
    }
}
*/
// This is an adaptor between subsystems (like the lexer/parser) and the diagnostic consumer. It
// holds the conumer and translator and facilitates creating Diagnostics, and ultimately
// provides the completed diagnostics to the DiagnosticConsumer.
pub struct DiagnosticEmitter<'c, 's, LocationT> {
    consumer: &'c mut dyn DiagnosticConsumer,
    translator: &'s dyn DiagnosticLocationTranslator<'s, LocationT>,
    diagnostic: Option<Diagnostic<'s>>,
}

impl<'c, 's, LocationT> DiagnosticEmitter<'c, 's, LocationT> {
    pub fn new(consumer: &'c mut dyn DiagnosticConsumer,
               translator: &'s dyn DiagnosticLocationTranslator<'s, LocationT>)
               -> DiagnosticEmitter<'c, 's, LocationT> {
        DiagnosticEmitter { consumer, translator, diagnostic: None }
    }

    pub fn get_location(&self, loc: &LocationT) -> DiagnosticLocation {
        self.translator.get_location(loc)
    }

    pub fn build(&mut self, level: DiagnosticLevel, kind: DiagnosticKind, location: &LocationT, body: String)
                -> &mut DiagnosticEmitter<'c, 's, LocationT> {
        debug_assert!(self.diagnostic.is_none(), "Repeated calls to build() without calling emit()");
        let loc = self.translator.get_location(location);
        let message = DiagnosticMessage { kind, location: loc, body };
        self.diagnostic = Some(Diagnostic::new(level, message));
        self
    }

    pub fn note(&mut self, kind: DiagnosticKind, location: &LocationT, body: String)
                -> &mut DiagnosticEmitter<'c, 's, LocationT> {
        let loc = self.translator.get_location(location);
        let message = DiagnosticMessage { kind, location: loc, body };
        self.diagnostic.as_mut().unwrap().notes.push(message);
        self
    }

    // Normally called by DiagnosticBuilder
    pub fn emit(&mut self) {
        let diag = self.diagnostic.as_mut().unwrap();
        self.consumer.handle_diagnostic(diag);
        self.diagnostic = None;
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