use std::str::Chars;

/// Peekable iterator over a char sequence.
///
/// Design roughly inspired by the rustc lexer Cursor.
pub struct Cursor<'a> {
    // An iterator over the input character string.
    chars: Chars<'a>,
    string: &'a str,
    bytes_remaining: usize,
    line: u32,
    column: u32,
}

impl<'a> Cursor<'a> {
    pub const EOF: char = '\0';

    pub fn new(input: &'a str) -> Cursor<'a> {
        Cursor {
            chars: input.chars(),
            string: input,
            bytes_remaining: input.len(),
            line: 1,
            column: 1,
        }
    }

    pub fn first(&self) -> char {
        self.chars.clone().next().unwrap_or(Self::EOF)
    }

    pub fn bump(&mut self) -> Option<char> {
        let c = self.chars.next()?;
        if c == '\n' {
            self.line += 1;
            self.column = 1;
        } else {
            self.column += 1;
        }
        Some(c)
    }

    pub fn is_eof(&self) -> bool {
        self.chars.as_str().is_empty()
    }

    pub fn extract_substring(&mut self) -> &'a str {
        let new_bytes_remaining = self.chars.as_str().len();
        let (prefix, suffix) = self.string.split_at(
            self.bytes_remaining - new_bytes_remaining);
        self.string = suffix;
        self.bytes_remaining = new_bytes_remaining;
        prefix
    }

    pub fn eat_while(&mut self, mut predicate: impl FnMut(char) -> bool) {
        while predicate(self.first()) && !self.is_eof() {
            self.bump();
        }
    }

    pub fn line(&self) -> u32 {
        self.line
    }

    pub fn column(&self) -> u32 {
        self.column
    }
}
