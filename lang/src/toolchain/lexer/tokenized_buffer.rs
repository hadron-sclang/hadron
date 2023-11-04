use super::cursor::Cursor;
use super::{Token, TokenIndex};
use crate::toolchain::diagnostics::diagnostic_emitter::DiagnosticConsumer;
use crate::toolchain::diagnostics::{DiagnosticLocation, DiagnosticLocationTranslator};
use crate::toolchain::source;

pub struct TokenizedBuffer<'s> {
    tokens: Vec<Token<'s>>,
    lines: Vec<&'s str>,
    source: &'s source::SourceBuffer<'s>,
}

impl<'s> TokenizedBuffer<'s> {
    pub fn tokenize(
        source: &'s source::SourceBuffer,
        diags: &mut impl DiagnosticConsumer,
    ) -> TokenizedBuffer<'s> {
        let mut lines = Vec::new();
        let cursor = Cursor::new(source, &mut lines, diags);
        let tokens = cursor.collect();
        TokenizedBuffer { tokens, lines, source }
    }

    pub fn token_at(&self, i: TokenIndex) -> Option<&Token<'s>> {
        self.tokens.get(i)
    }

    pub fn print_tokens(&self) {
        for token in self.tokens.iter() {
            println!("{:?}", &token);
        }
    }

    pub fn tokens(&self) -> &Vec<Token<'s>> {
        &self.tokens
    }
}

impl<'s> DiagnosticLocationTranslator<'s, TokenIndex> for TokenizedBuffer<'s> {
    fn get_location(&self, token_index: TokenIndex) -> DiagnosticLocation<'s> {
        let token = self.tokens[token_index];
        // Switch to zero-based line counting.
        let line_index = (token.line - 1) as usize;
        // TokenizedBuffer is only useful as a location translator after lexing is complete.
        // Otherwise, it is possible to encounter errors on the line currently being parsed, which
        // will not be present in the |self.lines| array.
        assert!(line_index < self.lines.len());
        DiagnosticLocation {
            file_name: self.source.file_name(),
            line_number: token.line,
            column_number: token.column,
            line: self.lines[line_index],
        }
    }
}
