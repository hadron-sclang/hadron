/// The enumerated type of all diagnostics Hadron emits.
#[derive(Clone, Copy)]
pub enum DiagnosticKind {
    SyntaxError { kind: SyntaxDiagnosticKind },

    ParseUnknownToken,
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
