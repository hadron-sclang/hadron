pub enum DiagnosticLevel {
    Note,
    Warning,
    Error,
}

pub enum DiagnosticKind {
    ParseError,
}

pub struct DiagnosticLocation<'a> {
    pub file_name: &'a str,
    pub line: &'a str,
    pub line_number: u32,
    pub column_number: u32,
}

pub struct DiagnosticMessage<'a> {
    pub kind: DiagnosticKind,
    pub location: DiagnosticLocation<'a>,
    pub format: &str,
}

pub struct Diagnostic<'a> {
    pub level: DiagnosticLevel,
    pub message: DiagnosticMessage<'a>,
    pub notes: std::Vec<DiagnosticMessage<'a>>,
}

pub trait DiagnosticConsumer {
    pub fn HandleDiagnostic(diag: Diagnostic);
    pub fn Flush();
}

