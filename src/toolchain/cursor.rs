use std::str::Chars;

/// Peekable iterator over a char sequence.
///
/// A close copy of the rustc lexer cursor of the same name, with some features removed.
pub struct Cursor<'a> {
    chars: Chars<'a>,
    counter: u32,
}

impl<'a> Cursor<'a> {
    pub const EOF: char = '\0';

    pub fn new(input: &'a str) -> Cursor<'a> {
        Cursor {
            chars: input.chars(),
            counter: 0
        }
    }

    pub fn first(&self) -> char {
        self.chars.clone().next().unwrap_or(Self::EOF)
    }

    pub fn bump(&mut self) -> Option<char> {
        let c = self.chars.next()?;
        self.counter += 1;
        Some(c)
    }

    pub fn is_eof(&self) -> bool {
        self.chars.as_str().is_empty()
    }

    pub fn counter(&self) -> u32 {
        self.counter
    }

    pub fn reset_counter(&mut self) {
        self.counter = 0;
    }

    pub fn eat_while(&mut self, mut predicate: impl FnMut(char) -> bool) {
        while predicate(self.first()) && !self.is_eof() {
            self.bump();
        }
    }
}
