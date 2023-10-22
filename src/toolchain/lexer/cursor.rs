use std::str::Chars;

use super::token::BinopKind;
use super::token::DelimiterKind;
use super::token::NumberKind;
use super::token::ReservedWordKind;
use super::token::Token;
use super::token::TokenKind;

/// Token iterator over a SourceBuffer.
///
/// Also tracks input buffer position by line and column.
///
/// Design roughly inspired by the rustc lexer Cursor.
pub struct Cursor<'s, 'v> {
    // An iterator over the input character string.
    chars: Chars<'s>,
    string: &'s str,
    bytes_remaining: usize,
    line: i32,
    column: i32,
    line_str: &'s str,
    line_bytes_remaining: usize,
    lines: &'v mut Vec<&'s str>,
}

impl<'s, 'v> Iterator for Cursor<'s, 'v> {
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
            '\'' => TokenKind::Symbol {
                has_escapes: self.scan_for_delimiter_or_escapes('\'')
            },

            // Inline literal symbols start with a '\'.
            '\\' => {
                self.eat_while(|c| c.is_alphanumeric() || c == '_');
                TokenKind::InlineSymbol
            },

            // Literal strings delimited with '"'.
            '"' => TokenKind::String {
                has_escapes: self.scan_for_delimiter_or_escapes('"')
            },

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
                    TokenKind::Delimiter { kind: DelimiterKind:: Underscore }
                }
            },

            // Character literals start with a $ and may be escaped.
            '$' => {
                let is_escaped = self.first() == '\\';
                self.bump();
                if is_escaped {
                    self.bump();
                }
                TokenKind::Character { is_escaped }
            },

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
            },

            // We coalesce unknown characters into a single Token, to cut down on the number of
            // Tokens that we lex from a string of garbage input.
            _ => {
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
                _ => TokenKind::Identifier
            };
            return Some(Token::new(identifier_kind, token_str, line, column));
        } else {
            return Some(Token::new(token_kind, token_str, line, column));
        }
    }
}

impl<'s, 'v> Cursor<'s, 'v> {
    pub const EOF: char = '\0';

    pub fn new(input: &'s str, lines: &'v mut Vec<&'s str>) -> Cursor<'s, 'v> {
        Cursor {
            chars: input.chars(),
            string: input,
            bytes_remaining: input.len(),
            line: 1,
            column: 1,

            line_str: input,
            line_bytes_remaining: input.len(),
            lines: lines,
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
                    self.lines.push(self.line_str);
                    self.line_str = "";
                }
                None
            },
            Some(c) => {
                // Handle newlines as we encounter them.
                if c == '\n' {
                    // Extract the line substring for the line we just terminated.
                    let new_bytes_remaining = self.chars.as_str().len();
                    let (prefix, suffix) = self.line_str.split_at(
                        self.line_bytes_remaining - new_bytes_remaining);
                    self.lines.push(prefix);
                    self.line_str = suffix;
                    self.line_bytes_remaining = new_bytes_remaining;

                    // Our 1-based line count should now be the same size as the lines array.
                    debug_assert!(self.line == self.lines.len().try_into().unwrap());
                    self.line += 1;
                    self.column = 1;
                } else {
                    self.column += 1;
                }
                Some(c)
            },
        }
    }

    fn is_eof(&self) -> bool {
        self.chars.as_str().is_empty()
    }

    fn extract_substring(&mut self) -> &'s str {
        let new_bytes_remaining = self.chars.as_str().len();
        let (prefix, suffix) = self.string.split_at(
            self.bytes_remaining - new_bytes_remaining);
        self.string = suffix;
        self.bytes_remaining = new_bytes_remaining;
        prefix
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
        TokenKind::LineComment
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

        TokenKind::BlockComment
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
                _ => TokenKind::Binop { kind: BinopKind::Identifier },
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
                '>' => TokenKind::Binop { kind: BinopKind::ReadWriteVar},
                _ => TokenKind::Binop { kind: BinopKind::Identifier }
            }
        }

        // Identifier binop, consume the rest of the characters.
        self.eat_while(|c| is_binop(c));
        TokenKind::Binop { kind: BinopKind::Identifier }
    }

    fn blank_space(&mut self) -> TokenKind {
        self.eat_while(|c| is_blank_space(c));
        TokenKind::BlankSpace
    }

    fn identifier(&mut self) -> TokenKind {
        self.eat_while(|c| is_identifier(c));
        // Any valid identifier terminated with a colon converts it into a keyword
        match self.first() {
            ':' => { self.bump(); TokenKind::Keyword }
            _ => TokenKind::Identifier
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
            if self.first() != c { break; }
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
            },

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
                    _ => false
                });

                if self.first() == '.' {
                    self.bump();
                    self.eat_while(|c| match c {
                        // Lowercase letters not allowed after the decimal in radix notation.
                        'A'..='Z' => true,
                        '0'..='9' => true,
                        _ => false
                    });
                    NumberKind::FloatRadix
                } else {
                    NumberKind::IntegerRadix
                }
            },

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
                if self.first() == '\\' { self.bump(); }
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
            _ => true
        }
    }
}

fn is_blank_space(c: char) -> bool {
    // Copied from the rustc lexer.
    matches!(c,
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
    matches!(c, '!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' |'?' | '/')
}

fn is_identifier(c: char) -> bool {
    c.is_alphanumeric() || c == '_'
}

