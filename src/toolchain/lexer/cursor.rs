use bstr;
use bstr::ByteSlice;

use crate::toolchain::diagnostics::diagnostic_emitter::Diagnostic;
use crate::toolchain::diagnostics::diagnostic_emitter::DiagnosticConsumer;
use crate::toolchain::diagnostics::diagnostic_emitter::DiagnosticLevel;
use crate::toolchain::diagnostics::diagnostic_emitter::DiagnosticMessage;
use crate::toolchain::diagnostics::diagnostic_kind::*;
use crate::toolchain::diagnostics::DiagnosticLocation;
use crate::toolchain::source::SourceBuffer;

use super::token::BinopKind;
use super::token::DelimiterKind;
use super::token::IgnoredKind;
use super::token::NumberKind;
use super::token::ReservedWordKind;
use super::token::Token;
use super::token::TokenKind;

/// Token iterator over a SourceBuffer.
///
/// Also tracks input buffer position by line and column.
///
/// Design roughly inspired by the rustc lexer Cursor.
pub struct Cursor<'s, 'v, 'd> {
    source: &'s SourceBuffer<'s>,
    // An iterator over the input character string.
    chars: bstr::Chars<'s>,
    string: &'s bstr::BStr,
    bytes_remaining: usize,
    line: i32,
    column: i32,
    line_str: &'s bstr::BStr,
    line_bytes_remaining: usize,
    lines: &'v mut Vec<&'s str>,
    diags: &'d mut dyn DiagnosticConsumer,
}

impl<'s, 'v, 'd> Iterator for Cursor<'s, 'v, 'd> {
    type Item = Token<'s>;

    fn next(&mut self) -> Option<Token<'s>> {
        // Collect string position at the start of the token.
        let line = self.line;
        let column = self.column;

        let first_char = match self.bump() {
            Some(c) => c,
            None => return None,
        };

        let token_kind = match first_char {
            // Blank spaces.
            c if is_blank_space(c) => self.blank_space(),

            // Identifiers starting with a lowercase letter, as do keywords.
            c if c.is_lowercase() => self.identifier(),

            // ClassNames starting with an uppercase letter.
            c if c.is_uppercase() => self.class_name(),

            // Literal numbers all start with a digit.
            '0'..='9' => self.number(),

            // Literal symbols delimited with '\''.
            '\'' => TokenKind::Symbol { has_escapes: self.scan_for_delimiter_or_escapes('\'') },

            // Inline literal symbols start with a '\'.
            '\\' => {
                self.eat_while(|c| c.is_alphanumeric() || c == '_');
                TokenKind::InlineSymbol
            }

            // Literal strings delimited with '"'.
            '"' => TokenKind::String { has_escapes: self.scan_for_delimiter_or_escapes('"') },

            // Single-character delimiters.
            '^' => TokenKind::Delimiter { kind: DelimiterKind::Caret },
            ':' => TokenKind::Delimiter { kind: DelimiterKind::Colon },
            ',' => TokenKind::Delimiter { kind: DelimiterKind::Comma },
            '(' => TokenKind::Delimiter { kind: DelimiterKind::ParenOpen },
            ')' => TokenKind::Delimiter { kind: DelimiterKind::ParenClose },
            '{' => TokenKind::Delimiter { kind: DelimiterKind::BraceOpen },
            '}' => TokenKind::Delimiter { kind: DelimiterKind::BraceClose },
            '[' => TokenKind::Delimiter { kind: DelimiterKind::BracketOpen },
            ']' => TokenKind::Delimiter { kind: DelimiterKind::BracketClose },
            '`' => TokenKind::Delimiter { kind: DelimiterKind::Grave },
            '#' => TokenKind::Delimiter { kind: DelimiterKind::Hash },
            '~' => TokenKind::Delimiter { kind: DelimiterKind::Tilde },
            ';' => TokenKind::Delimiter { kind: DelimiterKind::Semicolon },

            // Underscores can be single-character delimiters, but if they are followed by an
            // alphanumeric string or more underscores, we lex them as primitives.
            '_' => {
                if is_identifier(self.first()) {
                    self.eat_while(|c| is_identifier(c));
                    TokenKind::Primitive
                } else {
                    TokenKind::Delimiter { kind: DelimiterKind::Underscore }
                }
            }

            // Character literals start with a $ and may be escaped.
            '$' => {
                let is_escaped = self.first() == '\\';
                self.bump();
                if is_escaped {
                    self.bump();
                }
                TokenKind::Character { is_escaped }
            }

            // Several tokens could either be generic binop identifiers or have their own special
            // meaning. Disambiguate.
            c if is_binop(c) => self.binop_or_comment(c),

            '.' => {
                if self.first() != '.' {
                    TokenKind::Dot
                } else {
                    self.bump();
                    if self.first() != '.' {
                        TokenKind::DotDot
                    } else {
                        self.bump();
                        TokenKind::Ellipses
                    }
                }
            }

            Self::BAD => {
                // We manually build error messages in the lexer, as we don't yet have the file
                // completely mapped out for location translation to be meaningful.
                let location = DiagnosticLocation {
                    file_name: self.source.file_name(),
                    line_number: line,
                    column_number: column,
                    line: "",
                };
                let msg = DiagnosticMessage {
                    kind: DiagnosticKind::LexerError { kind: LexerDiagnosticKind::InvalidToken },
                    location,
                    body: "Invalid utf-8 sequence, input truncated.",
                };
                let diag = Diagnostic::new(DiagnosticLevel::Error, msg, Vec::new());
                self.diags.handle_diagnostic(diag);
                // Halt lexing and return the invalid token.
                return Some(Token::new(TokenKind::Invalid, "", line, column));
            }

            // We coalesce unknown characters into a single Token, to cut down on the number of
            // Tokens that we lex from a string of garbage input.
            _ => {
                let location = DiagnosticLocation {
                    file_name: self.source.file_name(),
                    line_number: line,
                    column_number: column,
                    line: "",
                };
                let msg = DiagnosticMessage {
                    kind: DiagnosticKind::LexerError { kind: LexerDiagnosticKind::InvalidToken },
                    location,
                    body: "Unrecognized character sequence.",
                };
                let diag = Diagnostic::new(DiagnosticLevel::Error, msg, Vec::new());
                self.diags.handle_diagnostic(diag);
                self.eat_while(|c| Self::is_unknown(c));
                TokenKind::Unknown
            }
        };

        // End of token, extract the substring.
        let token_str = self.extract_substring();

        // Fixup identifiers to match against reserved words
        if token_kind == TokenKind::Identifier {
            let identifier_kind = match token_str {
                "arg" => TokenKind::ReservedWord { kind: ReservedWordKind::Arg },
                "classvar" => TokenKind::ReservedWord { kind: ReservedWordKind::Classvar },
                "const" => TokenKind::ReservedWord { kind: ReservedWordKind::Const },
                "false" => TokenKind::ReservedWord { kind: ReservedWordKind::False },
                "inf" => TokenKind::ReservedWord { kind: ReservedWordKind::Inf },
                "nil" => TokenKind::ReservedWord { kind: ReservedWordKind::Nil },
                "pi" => TokenKind::ReservedWord { kind: ReservedWordKind::Pi },
                "true" => TokenKind::ReservedWord { kind: ReservedWordKind::True },
                "var" => TokenKind::ReservedWord { kind: ReservedWordKind::Var },
                _ => TokenKind::Identifier,
            };
            return Some(Token::new(identifier_kind, token_str, line, column));
        } else {
            return Some(Token::new(token_kind, token_str, line, column));
        }
    }
}

impl<'s, 'v, 'd> Cursor<'s, 'v, 'd> {
    pub const EOF: char = '\0';

    /// The bstr::Chars iterator substitutes invalid utf-8 sequences with the utf-8
    /// placeholder sequence U+FFFD. We treat the presence of this character as a signifier
    /// that the source string has invalid utf-8 characters. As rust requires &str elements
    /// to always contain valid utf-8 only, this is necessarily a fatal lexing error.
    pub const BAD: char = '\u{fffd}';

    pub fn new(
        source: &'s SourceBuffer<'s>,
        lines: &'v mut Vec<&'s str>,
        diags: &'d mut impl DiagnosticConsumer,
    ) -> Cursor<'s, 'v, 'd> {
        let input = source.code();
        Cursor {
            source,
            chars: input.chars(),
            string: input,
            bytes_remaining: input.len(),
            line: 1,
            column: 1,

            line_str: input,
            line_bytes_remaining: input.len(),
            lines: lines,
            diags,
        }
    }

    fn first(&self) -> char {
        self.chars.clone().next().unwrap_or(Self::EOF)
    }

    fn bump(&mut self) -> Option<char> {
        let next = self.chars.next();
        match next {
            None => {
                if self.line_str.len() > 0 {
                    // We are confident that line_str is valid utf-8 because we have just checked
                    // every code point for validity while parsing.
                    let line_str = unsafe { self.line_str.to_str_unchecked() };
                    self.lines.push(line_str);
                    self.line_str = bstr::BStr::new(&[]);
                }
                None
            }
            Some(c) if c == Self::BAD => {
                // Invalidate chars iterator, this is the end of the stream.
                self.chars = bstr::BStr::new(&[]).chars();
                Some(Self::BAD)
            }
            Some(c) => {
                // Handle newlines as we encounter them.
                if c == '\n' {
                    // Extract the line substring for the line we just terminated.
                    let new_bytes_remaining = self.chars.as_bytes().len();
                    let (prefix, suffix) =
                        self.line_str.split_at(self.line_bytes_remaining - new_bytes_remaining);

                    // We just checked every code point in prefix for validity so this does not
                    // break the requirement that a &str must always reference valid utf-8.
                    let prefix_str = unsafe { prefix.to_str_unchecked() };

                    self.lines.push(prefix_str);
                    self.line_str = bstr::BStr::new(suffix);
                    self.line_bytes_remaining = new_bytes_remaining;

                    // Our 1-based line count should now be the same size as the lines array.
                    debug_assert!(self.line == self.lines.len().try_into().unwrap());
                    self.line += 1;
                    self.column = 1;
                } else {
                    self.column += 1;
                }
                Some(c)
            }
        }
    }

    fn is_eof(&self) -> bool {
        self.chars.as_bytes().is_empty()
    }

    fn extract_substring(&mut self) -> &'s str {
        let new_bytes_remaining = self.chars.as_bytes().len();
        let (prefix, suffix) = self.string.split_at(self.bytes_remaining - new_bytes_remaining);
        self.string = bstr::BStr::new(suffix);
        self.bytes_remaining = new_bytes_remaining;
        let prefix_str = unsafe { prefix.to_str_unchecked() };
        prefix_str
    }

    fn eat_while(&mut self, mut predicate: impl FnMut(char) -> bool) {
        while predicate(self.first()) && !self.is_eof() {
            self.bump();
        }
    }

    // Note that line comments will ignore any block comments that start subsequently on that line.
    fn line_comment(&mut self) -> TokenKind {
        self.eat_while(|c| c != '\n');
        // Consume the line end character.
        self.bump();
        TokenKind::Ignored { kind: IgnoredKind::LineComment }
    }

    fn block_comment(&mut self) -> TokenKind {
        let mut depth = 1usize;
        while let Some(c) = self.bump() {
            match c {
                '/' if self.first() == '*' => {
                    self.bump();
                    depth += 1;
                }
                '*' if self.first() == '/' => {
                    self.bump();
                    depth -= 1;
                    if depth == 0 {
                        break;
                    }
                }
                _ => (),
            }
        }

        TokenKind::Ignored { kind: IgnoredKind::BlockComment }
    }

    fn binop_or_comment(&mut self, first: char) -> TokenKind {
        // Most interesting binops are a single character in length. Check for those first.
        if !is_binop(self.first()) {
            return match first {
                '*' => TokenKind::Binop { kind: BinopKind::Asterisk },
                '=' => TokenKind::Binop { kind: BinopKind::Assign },
                '>' => TokenKind::Binop { kind: BinopKind::GreaterThan },
                '<' => TokenKind::Binop { kind: BinopKind::LessThan },
                '-' => TokenKind::Binop { kind: BinopKind::Minus },
                '|' => TokenKind::Binop { kind: BinopKind::Pipe },
                '+' => TokenKind::Binop { kind: BinopKind::Plus },
                _ => TokenKind::Binop { kind: BinopKind::BinopIdentifier },
            };
        }

        // Next character is a binop character too, extract.
        let next = self.bump().expect("Expecting a valid binop character.");

        // Block and line comments are only lexed as such if they are the first characters
        // in the sequence of binop characters.
        if first == '/' {
            if next == '/' {
                return self.line_comment();
            }
            if next == '*' {
                return self.block_comment();
            }
        }

        // Check for two-character binops that start with '<'.
        if first == '<' && !is_binop(self.first()) {
            return match next {
                '-' => TokenKind::Binop { kind: BinopKind::LeftArrow },
                '>' => TokenKind::Binop { kind: BinopKind::ReadWriteVar },
                _ => TokenKind::Binop { kind: BinopKind::BinopIdentifier },
            };
        }

        // Identifier binop, consume the rest of the characters.
        self.eat_while(|c| is_binop(c));
        TokenKind::Binop { kind: BinopKind::BinopIdentifier }
    }

    fn blank_space(&mut self) -> TokenKind {
        self.eat_while(|c| is_blank_space(c));
        TokenKind::Ignored { kind: IgnoredKind::BlankSpace }
    }

    fn identifier(&mut self) -> TokenKind {
        self.eat_while(|c| is_identifier(c));
        // Any valid identifier terminated with a colon converts it into a keyword
        match self.first() {
            ':' => {
                self.bump();
                TokenKind::Keyword
            }
            _ => TokenKind::Identifier,
        }
    }

    fn class_name(&mut self) -> TokenKind {
        self.eat_while(|c| c.is_alphanumeric() || c == '_');
        TokenKind::ClassName
    }

    fn accidental(&mut self, c: char) -> NumberKind {
        let mut count = 1;
        while count <= 4 {
            self.bump();
            if self.first() != c {
                break;
            }
            count += 1;
        }
        if count == 1 && self.first().is_numeric() {
            self.eat_while(|c| c.is_numeric());
            NumberKind::FloatAccidentalCents
        } else {
            NumberKind::FloatAccidental
        }
    }

    fn number(&mut self) -> TokenKind {
        // Every number starts with one or more numeric characters.
        self.eat_while(|c| c.is_numeric());

        let number_kind = match self.first() {
            // We indicate hexadecimal numbers with a lowercase x.
            'x' => {
                self.bump();
                self.eat_while(|c| match c {
                    'a'..='f' => true,
                    'A'..='F' => true,
                    '0'..='9' => true,
                    _ => false,
                });
                NumberKind::IntegerHex
            }

            // 1-4 'b' characters or 1 'b' character followed by a number makes a flat float.
            'b' => self.accidental('b'),

            // 1-4 's' characters or 1 's' character followed by a number makes a sharp float
            's' => self.accidental('s'),

            'r' => {
                self.bump();
                self.eat_while(|c| match c {
                    'a'..='z' => true,
                    'A'..='Z' => true,
                    '0'..='9' => true,
                    _ => false,
                });

                if self.first() == '.' {
                    self.bump();
                    self.eat_while(|c| match c {
                        // Lowercase letters not allowed after the decimal in radix notation.
                        'A'..='Z' => true,
                        '0'..='9' => true,
                        _ => false,
                    });
                    NumberKind::FloatRadix
                } else {
                    NumberKind::IntegerRadix
                }
            }

            '.' => {
                self.bump();
                self.eat_while(|c| c.is_numeric());
                if self.first() == 'e' {
                    // Consume the 'e'
                    self.bump();
                    if self.first() == '+' || self.first() == '-' {
                        self.bump();
                    }
                    self.eat_while(|c| c.is_numeric());
                    NumberKind::FloatSci
                } else {
                    NumberKind::Float
                }
            }

            'e' => {
                self.bump();
                if self.first() == '+' || self.first() == '-' {
                    self.bump();
                }
                self.eat_while(|c| c.is_numeric());
                NumberKind::FloatSci
            }

            _ => NumberKind::Integer,
        };

        TokenKind::Number { kind: number_kind }
    }

    fn scan_for_delimiter_or_escapes(&mut self, delimiter: char) -> bool {
        self.eat_while(|c| c != '\\' && c != delimiter);
        if self.first() == delimiter {
            self.bump();
            false
        } else {
            while !self.is_eof() && self.first() != delimiter {
                if self.first() == '\\' {
                    self.bump();
                }
                self.bump();
                self.eat_while(|c| c != '\\' && c != delimiter);
            }
            self.bump();
            true
        }
    }

    fn is_unknown(c: char) -> bool {
        match c {
            u if is_blank_space(u) => false,
            u if is_identifier(u) => false,
            u if u.is_uppercase() => false,
            u if is_binop(u) => false,
            '^' => false,
            ':' => false,
            ',' => false,
            '(' => false,
            ')' => false,
            '{' => false,
            '}' => false,
            '[' => false,
            ']' => false,
            '`' => false,
            '#' => false,
            '~' => false,
            ';' => false,
            '$' => false,
            '.' => false,
            _ => true,
        }
    }
}

fn is_blank_space(c: char) -> bool {
    // Copied from the rustc lexer.
    matches!(
        c,
        // Usual ASCII suspects
        '\u{0009}'   // \t
        | '\u{000A}' // \n
        | '\u{000B}' // vertical tab
        | '\u{000C}' // form feed
        | '\u{000D}' // \r
        | '\u{0020}' // space

        // NEXT LINE from latin1
        | '\u{0085}'

        // Bidi markers
        | '\u{200E}' // LEFT-TO-RIGHT MARK
        | '\u{200F}' // RIGHT-TO-LEFT MARK

        // Dedicated whitespace characters from Unicode
        | '\u{2028}' // LINE SEPARATOR
        | '\u{2029}' // PARAGRAPH SEPARATOR
    )
}

fn is_binop(c: char) -> bool {
    matches!(c, '!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' | '?' | '/')
}

fn is_identifier(c: char) -> bool {
    c.is_alphanumeric() || c == '_'
}
