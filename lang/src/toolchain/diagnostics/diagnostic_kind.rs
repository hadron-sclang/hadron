/// The enumerated type of all diagnostics Hadron emits.
#[derive(Clone, Copy)]
pub enum DiagnosticKind {
    LexerError { kind: LexerDiagnosticKind },
    SyntaxError { kind: SyntaxDiagnosticKind },
}

#[derive(Clone, Copy)]
pub enum LexerDiagnosticKind {
    /// Token doesn't match any defined pattern.
    UnknownToken,

    /// Token is not valid utf-8, halting lexing.
    InvalidToken,
}

#[derive(Clone, Copy)]
pub enum SyntaxDiagnosticKind {
    /// We were expecting one particular thing and didn't encounter it.
    MissingToken,

    /// The parser was expecting a closing brace, bracket, or parenthesis for an opening one,
    /// and will synthesize it and keep going.
    UnclosedPair,

    /// We've encountered something we really don't know what to make of.
    UnexpectedToken,

    /// Unexpected end of input, we were in the middle of parsing something.
    UnexpectedEndOfInput,
}
