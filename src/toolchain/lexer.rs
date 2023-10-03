//! A SuperCollider language lexer, which splits the input sclang string into tokens for parsing.
//!
//! This lexer provides light-weight [Token] structures via the iterator provided by the
//! [tokenize()] method. In addition to the parser front end, this lexer intends to support other
//! use cases like tokenization for an LSP server or other user-in-the-loop runtime analysis
//! services. To do that, it must be fast and tolerant of erroneous or incomplete input. The lexer
//! does no validation or processing of the input string other than tokenization; for example, it
//! does not convert number strings into their binary representation. The compiler defers further
//! work until the parsing or semantic translation steps.
//!
//! The rustc lexer inspired the initial design of this lexer, so credit is due to them.
//!

use crate::toolchain::cursor::Cursor;

/// Represents a single lexical token of SuperCollider language.
///
/// Because the lexer considers blank space as a [TokenKind::BlankSpace] token, and unrecognized
/// characters as [TokenKind::Unknown] tokens, every character in the input string is covered by
/// a [Token].
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Token<'a> {
    /// The kind of Token.
    pub kind: TokenKind,

    // The substring representing the Token.
    pub string: &'a str,

    // The 1-based line position in the input string.
    pub line: u32,

    // The 1-based character column on the line.
    pub column: u32,
}

impl<'a> Token<'a> {
    fn new(kind: TokenKind, string: &'a str, line: u32, column: u32) -> Token<'a> {
        Token { kind, string, line, column }
    }

    fn end() -> Token<'a> {
        Token { kind: TokenKind::EndOfInput, string: "", line: 0, column: 0 }
    }
}

/// An enumeration of all possible Token types in SuperCollider.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TokenKind {
    /// A sequence of binop characters, possibly with special meaning.
    ///
    /// SuperCollider lexes *binops* (or binary operators) as one or more characters from the set:
    ///
    /// `'!' | '@' | '%' | '&' | '*' | '-' | '+' | '=' | '|' | '<' | '>' |'?' | '/'`
    ///
    /// Some binops have special meaning while parsing, so we identify those with [BinopKind].
    Binop { kind: BinopKind },

    /// Blank space, including some extended characters like the vertical tab. See
    /// `is_blank_space()` for details.
    BlankSpace,

    /// A block comment, including any nested comments within. It may contain line breaks.
    BlockComment,

    /// A character literal, such as `$a` or `$\t`. `is_escaped` is `false` in the former example,
    /// `true` in the latter.
    Character { is_escaped: bool },

    /// A class name starting with an uppercase letter, such as `SynthDef`.
    ClassName,

    Delimiter { kind: DelimiterKind },

    /// A single period.
    Dot,

    /// Two periods.
    DotDot,

    /// Three periods.
    Ellipses,

    /// This internal [TokenKind] is for signaling the end of the stream to the iterator. It should
    /// not appear to [Token] consumers.
    EndOfInput,

    /// Any name starting with a lowercase letter and followed by 0 or more alphanumeric letters or
    /// underscore `_`.
    Identifier,

    /// A symbol starting with a forward slash, such as `\synth`.
    InlineSymbol,

    /// An identifier followed by a colon, `add:` for example.
    Keyword,

    /// A double-slash comment terminated by the end of the line or input.
    LineComment,

    /// A numeric literal, either floating-point or integer. The [NumberKind] gives a hint about
    /// how to convert it to machine representation.
    Number { kind: NumberKind },

    /// A binding to a C++ function in legacy SuperCollider. An underscore followed by one or more
    /// valid identifier characters (alphanumeric or underscore). For example, `_Basic_New`.
    Primitive,

    ReservedWord { kind: ReservedWordKind },

    /// A double-quoted character literal sequence. If it has backslash (`\`) escape characters in
    /// it `has_escapes` is true, telling later stages if they must process the string more or can
    /// copy it directly to the string literal.
    String { has_escapes: bool },

    /// A single-quoted character literal sequence.
    Symbol { has_escapes: bool },

    /// Anything that the lexer didn't recognize as valid SuperCollider language input.
    Unknown
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BinopKind {
    /// The `=` operator.
    Assign,

    /// The `*` operator.
    Asterisk,

    /// A non-specific binop that didn't match the reserved binop patterns, `+/+` for example.
    Identifier,

    /// The `>` operator.
    GreaterThan,

    /// The `<-` operator.
    LeftArrow,

    /// The `<` operator.
    LessThan,

    /// The `-` operator.
    Minus,

    /// The `|` operator.
    Pipe,

    /// The `+` operator.
    Plus,

    /// The `<>` operator.
    ReadWriteVar,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DelimiterKind {
    /// `}` single-character delimiter.
    BraceClose,

    /// `{` single-character delimiter.
    BraceOpen,

    /// `]` single-character delimiter.
    BracketClose,

    /// `[` single-character delimiter.
    BracketOpen,

    /// `^` single-character delimiter.
    Caret,

    /// `:` single-character delimiter.
    Colon,

    /// `,` single-character delimiter.
    Comma,

    /// \` single-character delimiter.
    Grave,

    /// `#` single-character delimiter.
    Hash,

    /// `)` single-character delimiter.
    ParenClose,

    /// `(` single-character delimiter.
    ParenOpen,

    /// `;` single-character delimiter.
    Semicolon,

    /// `~` single-character delimiter.
    Tilde,

    /// `_` single-character delimiter.
    Underscore,
}


#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum NumberKind {
    /// A base-10 number with a single dot, `1.0` for example.
    Float,

    /// A base-10 number followed by 1-3 `b` or `s` characters, `10bb` for example.
    FloatAccidental,

    /// A base-10 number followed by a `b` or `s` character and a cent number, `10b499` for
    /// example.
    FloatAccidentalCents,

    /// A base-10 radix number followed by an `r` and then a radix floating point number,
    /// `36rSUPER.C0LLIDER` for example.
    FloatRadix,

    /// A base-10 radix floating point number followed by an `e` and exponent number.
    FloatSci,

    /// A base-10 integer number, `0` for example.
    Integer,

    /// A base-10 number followed by an `x` and then a base-16 integer number, `0xdeadbeef` for
    /// example.
    IntegerHex,

    /// A base-10 number followed by an `r` and then a radix integer number, `2r100` for example.
    IntegerRadix,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ReservedWordKind {
    /// `arg` declares method and function arguments.
    Arg,

    /// `classvar` marks a variable as belonging to the class.
    Classvar,

    /// `const` marks a class member as read-only.
    Const,

    /// `false` is a constant boolean type.
    False,

    /// `inf` is a constant float value representing infinity.
    Inf,

    /// `nil` is the constant nil type.
    Nil,

    /// `pi` is a constant float value representing pi.
    Pi,

    /// `true` is a constant boolean type.
    True,

    /// `var` declares local and object instance variables.
    Var
}

/// Returns a [Token] iterator over the `input` string.
pub fn tokenize<'a>(input: &'a str) -> impl Iterator<Item = Token<'a>> + '_ {
    let mut cursor = Cursor::new(input);
    std::iter::from_fn(move || {
        let token = cursor.advance_token();
        if token.kind != TokenKind::EndOfInput { Some(token) } else { None }
    })
}

impl<'a> Cursor<'a> {
    pub fn advance_token(&mut self) -> Token<'a> {
        // Collect string position at the start of the token.
        let line = self.line();
        let column = self.column();

        let first_char = match self.bump() {
            Some(c) => c,
            None => return Token::end(),
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

            _ => TokenKind::Unknown
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
            Token::new(identifier_kind, token_str, line, column)
        } else {
            Token::new(token_kind, token_str, line, column)
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

#[cfg(test)]
mod tests {
    use super::tokenize;

    /// Lexing helper function to compare expected lexing to a provided debug string of the tokens.
    ///
    /// Prepends a carriage return in front of each token, for readability of the test code.
    ///
    /// Note that the debug printer escapes `"` and `\` characters inside strings, so for example:
    ///
    /// ```
    ///         check_lexing(r#""\"""#, r#"
    ///Token { kind: String { has_escapes: true }, string: "\"\\\"\"", line: 1, column: 1 }"#);
    /// ```
    /// Will pass.
    ///
    /// The input is a string `"\""` which is escaped by the `format!` call into the output
    /// `\"\\\"\"`. In a failed test, the entire Token string is again escaped, meaning that the
    /// contents of string will be *double* escaped when printed in the test failure.
    fn check_lexing(src: &str, expect: &str) {
        let actual: String = tokenize(src).map(|token| format!("\n{:?}", token)).collect();
        assert_eq!(expect, &actual);
    }

    #[test]
    fn smoke_test() {
        check_lexing(r#"SynthDef(\a, { var snd = SinOsc.ar(440, mul: 0.5); snd; });"#, r#"
Token { kind: ClassName, string: "SynthDef", line: 1, column: 1 }
Token { kind: Delimiter { kind: ParenOpen }, string: "(", line: 1, column: 9 }
Token { kind: InlineSymbol, string: "\\a", line: 1, column: 10 }
Token { kind: Delimiter { kind: Comma }, string: ",", line: 1, column: 12 }
Token { kind: BlankSpace, string: " ", line: 1, column: 13 }
Token { kind: Delimiter { kind: BraceOpen }, string: "{", line: 1, column: 14 }
Token { kind: BlankSpace, string: " ", line: 1, column: 15 }
Token { kind: ReservedWord { kind: Var }, string: "var", line: 1, column: 16 }
Token { kind: BlankSpace, string: " ", line: 1, column: 19 }
Token { kind: Identifier, string: "snd", line: 1, column: 20 }
Token { kind: BlankSpace, string: " ", line: 1, column: 23 }
Token { kind: Binop { kind: Assign }, string: "=", line: 1, column: 24 }
Token { kind: BlankSpace, string: " ", line: 1, column: 25 }
Token { kind: ClassName, string: "SinOsc", line: 1, column: 26 }
Token { kind: Dot, string: ".", line: 1, column: 32 }
Token { kind: Identifier, string: "ar", line: 1, column: 33 }
Token { kind: Delimiter { kind: ParenOpen }, string: "(", line: 1, column: 35 }
Token { kind: Number { kind: Integer }, string: "440", line: 1, column: 36 }
Token { kind: Delimiter { kind: Comma }, string: ",", line: 1, column: 39 }
Token { kind: BlankSpace, string: " ", line: 1, column: 40 }
Token { kind: Keyword, string: "mul:", line: 1, column: 41 }
Token { kind: BlankSpace, string: " ", line: 1, column: 45 }
Token { kind: Number { kind: Float }, string: "0.5", line: 1, column: 46 }
Token { kind: Delimiter { kind: ParenClose }, string: ")", line: 1, column: 49 }
Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 50 }
Token { kind: BlankSpace, string: " ", line: 1, column: 51 }
Token { kind: Identifier, string: "snd", line: 1, column: 52 }
Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 55 }
Token { kind: BlankSpace, string: " ", line: 1, column: 56 }
Token { kind: Delimiter { kind: BraceClose }, string: "}", line: 1, column: 57 }
Token { kind: Delimiter { kind: ParenClose }, string: ")", line: 1, column: 58 }
Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 59 }"#);
    }

    #[test]
    fn binops() {
        // Check the special binops.
        check_lexing("= * > <- < - | + <>", r#"
Token { kind: Binop { kind: Assign }, string: "=", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 2 }
Token { kind: Binop { kind: Asterisk }, string: "*", line: 1, column: 3 }
Token { kind: BlankSpace, string: " ", line: 1, column: 4 }
Token { kind: Binop { kind: GreaterThan }, string: ">", line: 1, column: 5 }
Token { kind: BlankSpace, string: " ", line: 1, column: 6 }
Token { kind: Binop { kind: LeftArrow }, string: "<-", line: 1, column: 7 }
Token { kind: BlankSpace, string: " ", line: 1, column: 9 }
Token { kind: Binop { kind: LessThan }, string: "<", line: 1, column: 10 }
Token { kind: BlankSpace, string: " ", line: 1, column: 11 }
Token { kind: Binop { kind: Minus }, string: "-", line: 1, column: 12 }
Token { kind: BlankSpace, string: " ", line: 1, column: 13 }
Token { kind: Binop { kind: Pipe }, string: "|", line: 1, column: 14 }
Token { kind: BlankSpace, string: " ", line: 1, column: 15 }
Token { kind: Binop { kind: Plus }, string: "+", line: 1, column: 16 }
Token { kind: BlankSpace, string: " ", line: 1, column: 17 }
Token { kind: Binop { kind: ReadWriteVar }, string: "<>", line: 1, column: 18 }"#);

        // Check that the lexer doesn't get confused on binops with special prefixes.
        check_lexing("== ** >< <-- << -- || ++ <><", r#"
Token { kind: Binop { kind: Identifier }, string: "==", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 3 }
Token { kind: Binop { kind: Identifier }, string: "**", line: 1, column: 4 }
Token { kind: BlankSpace, string: " ", line: 1, column: 6 }
Token { kind: Binop { kind: Identifier }, string: "><", line: 1, column: 7 }
Token { kind: BlankSpace, string: " ", line: 1, column: 9 }
Token { kind: Binop { kind: Identifier }, string: "<--", line: 1, column: 10 }
Token { kind: BlankSpace, string: " ", line: 1, column: 13 }
Token { kind: Binop { kind: Identifier }, string: "<<", line: 1, column: 14 }
Token { kind: BlankSpace, string: " ", line: 1, column: 16 }
Token { kind: Binop { kind: Identifier }, string: "--", line: 1, column: 17 }
Token { kind: BlankSpace, string: " ", line: 1, column: 19 }
Token { kind: Binop { kind: Identifier }, string: "||", line: 1, column: 20 }
Token { kind: BlankSpace, string: " ", line: 1, column: 22 }
Token { kind: Binop { kind: Identifier }, string: "++", line: 1, column: 23 }
Token { kind: BlankSpace, string: " ", line: 1, column: 25 }
Token { kind: Binop { kind: Identifier }, string: "<><", line: 1, column: 26 }"#);

        // Comment patterns only lexed as comments at the start of a binop token.
        check_lexing(r#"+//*"#, r#"
Token { kind: Binop { kind: Identifier }, string: "+//*", line: 1, column: 1 }"#);
    }

    #[test]
    fn block_comments() {
        // Simple inline comments.
        check_lexing("/**/ /*/ hello */", r#"
Token { kind: BlockComment, string: "/**/", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 5 }
Token { kind: BlockComment, string: "/*/ hello */", line: 1, column: 6 }"#);

        // Commented out code.
        check_lexing(r#"/* var a = 2; */"#, r#"
Token { kind: BlockComment, string: "/* var a = 2; */", line: 1, column: 1 }"#);

        // Nested comments.
        check_lexing(r#"/* / * / * / * /** /**/ // * / * **/*/"#, r#"
Token { kind: BlockComment, string: "/* / * / * / * /** /**/ // * / * **/*/", line: 1, column: 1 }"#);
    }

    #[test]
    fn characters() {
        check_lexing(r#"$\t$a$$$ $""#, r#"
Token { kind: Character { is_escaped: true }, string: "$\\t", line: 1, column: 1 }
Token { kind: Character { is_escaped: false }, string: "$a", line: 1, column: 4 }
Token { kind: Character { is_escaped: false }, string: "$$", line: 1, column: 6 }
Token { kind: Character { is_escaped: false }, string: "$ ", line: 1, column: 8 }
Token { kind: Character { is_escaped: false }, string: "$\"", line: 1, column: 10 }"#);
    }

    #[test]
    fn class_names() {
        check_lexing("A B C SinOsc Main_32a", r#"
Token { kind: ClassName, string: "A", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 2 }
Token { kind: ClassName, string: "B", line: 1, column: 3 }
Token { kind: BlankSpace, string: " ", line: 1, column: 4 }
Token { kind: ClassName, string: "C", line: 1, column: 5 }
Token { kind: BlankSpace, string: " ", line: 1, column: 6 }
Token { kind: ClassName, string: "SinOsc", line: 1, column: 7 }
Token { kind: BlankSpace, string: " ", line: 1, column: 13 }
Token { kind: ClassName, string: "Main_32a", line: 1, column: 14 }"#);
    }

    #[test]
    fn delimiters() {
        check_lexing(r#"^:,(){}[]`#~_;"#, r##"
Token { kind: Delimiter { kind: Caret }, string: "^", line: 1, column: 1 }
Token { kind: Delimiter { kind: Colon }, string: ":", line: 1, column: 2 }
Token { kind: Delimiter { kind: Comma }, string: ",", line: 1, column: 3 }
Token { kind: Delimiter { kind: ParenOpen }, string: "(", line: 1, column: 4 }
Token { kind: Delimiter { kind: ParenClose }, string: ")", line: 1, column: 5 }
Token { kind: Delimiter { kind: BraceOpen }, string: "{", line: 1, column: 6 }
Token { kind: Delimiter { kind: BraceClose }, string: "}", line: 1, column: 7 }
Token { kind: Delimiter { kind: BracketOpen }, string: "[", line: 1, column: 8 }
Token { kind: Delimiter { kind: BracketClose }, string: "]", line: 1, column: 9 }
Token { kind: Delimiter { kind: Grave }, string: "`", line: 1, column: 10 }
Token { kind: Delimiter { kind: Hash }, string: "#", line: 1, column: 11 }
Token { kind: Delimiter { kind: Tilde }, string: "~", line: 1, column: 12 }
Token { kind: Delimiter { kind: Underscore }, string: "_", line: 1, column: 13 }
Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 14 }"##);
    }


    #[test]
    fn dots() {
        check_lexing(". .. ... .... ..... ......", r#"
Token { kind: Dot, string: ".", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 2 }
Token { kind: DotDot, string: "..", line: 1, column: 3 }
Token { kind: BlankSpace, string: " ", line: 1, column: 5 }
Token { kind: Ellipses, string: "...", line: 1, column: 6 }
Token { kind: BlankSpace, string: " ", line: 1, column: 9 }
Token { kind: Ellipses, string: "...", line: 1, column: 10 }
Token { kind: Dot, string: ".", line: 1, column: 13 }
Token { kind: BlankSpace, string: " ", line: 1, column: 14 }
Token { kind: Ellipses, string: "...", line: 1, column: 15 }
Token { kind: DotDot, string: "..", line: 1, column: 18 }
Token { kind: BlankSpace, string: " ", line: 1, column: 20 }
Token { kind: Ellipses, string: "...", line: 1, column: 21 }
Token { kind: Ellipses, string: "...", line: 1, column: 24 }"#);
    }

    #[test]
    fn identifiers() {
        check_lexing("a b c if while const classvar zz_Top3", r#"
Token { kind: Identifier, string: "a", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 2 }
Token { kind: Identifier, string: "b", line: 1, column: 3 }
Token { kind: BlankSpace, string: " ", line: 1, column: 4 }
Token { kind: Identifier, string: "c", line: 1, column: 5 }
Token { kind: BlankSpace, string: " ", line: 1, column: 6 }
Token { kind: Identifier, string: "if", line: 1, column: 7 }
Token { kind: BlankSpace, string: " ", line: 1, column: 9 }
Token { kind: Identifier, string: "while", line: 1, column: 10 }
Token { kind: BlankSpace, string: " ", line: 1, column: 15 }
Token { kind: ReservedWord { kind: Const }, string: "const", line: 1, column: 16 }
Token { kind: BlankSpace, string: " ", line: 1, column: 21 }
Token { kind: ReservedWord { kind: Classvar }, string: "classvar", line: 1, column: 22 }
Token { kind: BlankSpace, string: " ", line: 1, column: 30 }
Token { kind: Identifier, string: "zz_Top3", line: 1, column: 31 }"#);

        check_lexing("mul: add:",r#"
Token { kind: Keyword, string: "mul:", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 5 }
Token { kind: Keyword, string: "add:", line: 1, column: 6 }"#);
    }

    #[test]
    fn inline_symbols() {
        check_lexing(r#"\a \b \c \1 \ \_ \A_l0ng3r_SYMBOL"#, r#"
Token { kind: InlineSymbol, string: "\\a", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 3 }
Token { kind: InlineSymbol, string: "\\b", line: 1, column: 4 }
Token { kind: BlankSpace, string: " ", line: 1, column: 6 }
Token { kind: InlineSymbol, string: "\\c", line: 1, column: 7 }
Token { kind: BlankSpace, string: " ", line: 1, column: 9 }
Token { kind: InlineSymbol, string: "\\1", line: 1, column: 10 }
Token { kind: BlankSpace, string: " ", line: 1, column: 12 }
Token { kind: InlineSymbol, string: "\\", line: 1, column: 13 }
Token { kind: BlankSpace, string: " ", line: 1, column: 14 }
Token { kind: InlineSymbol, string: "\\_", line: 1, column: 15 }
Token { kind: BlankSpace, string: " ", line: 1, column: 17 }
Token { kind: InlineSymbol, string: "\\A_l0ng3r_SYMBOL", line: 1, column: 18 }"#);
    }

    #[test]
    fn line_comments() {
        check_lexing(r#"// start of line
// /* start of block
        // end of line"#, r#"
Token { kind: LineComment, string: "// start of line\n", line: 1, column: 1 }
Token { kind: LineComment, string: "// /* start of block\n", line: 2, column: 1 }
Token { kind: BlankSpace, string: "        ", line: 3, column: 1 }
Token { kind: LineComment, string: "// end of line", line: 3, column: 9 }"#);
    }

    #[test]
    fn numbers() {
        check_lexing(r#"123 0x1aAfF 36rZIGZAG10
100.100 4bbb 23s200 16rA.F 1.7e-9 1e+7"#, r#"
Token { kind: Number { kind: Integer }, string: "123", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 4 }
Token { kind: Number { kind: IntegerHex }, string: "0x1aAfF", line: 1, column: 5 }
Token { kind: BlankSpace, string: " ", line: 1, column: 12 }
Token { kind: Number { kind: IntegerRadix }, string: "36rZIGZAG10", line: 1, column: 13 }
Token { kind: BlankSpace, string: "\n", line: 1, column: 24 }
Token { kind: Number { kind: Float }, string: "100.100", line: 2, column: 1 }
Token { kind: BlankSpace, string: " ", line: 2, column: 8 }
Token { kind: Number { kind: FloatAccidental }, string: "4bbb", line: 2, column: 9 }
Token { kind: BlankSpace, string: " ", line: 2, column: 13 }
Token { kind: Number { kind: FloatAccidentalCents }, string: "23s200", line: 2, column: 14 }
Token { kind: BlankSpace, string: " ", line: 2, column: 20 }
Token { kind: Number { kind: FloatRadix }, string: "16rA.F", line: 2, column: 21 }
Token { kind: BlankSpace, string: " ", line: 2, column: 27 }
Token { kind: Number { kind: FloatSci }, string: "1.7e-9", line: 2, column: 28 }
Token { kind: BlankSpace, string: " ", line: 2, column: 34 }
Token { kind: Number { kind: FloatSci }, string: "1e+7", line: 2, column: 35 }"#);
    }

    #[test]
    fn primitives() {
        check_lexing("_abc123 _Test_ _PrimitiTooTaah", r#"
Token { kind: Primitive, string: "_abc123", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 8 }
Token { kind: Primitive, string: "_Test_", line: 1, column: 9 }
Token { kind: BlankSpace, string: " ", line: 1, column: 15 }
Token { kind: Primitive, string: "_PrimitiTooTaah", line: 1, column: 16 }"#);
    }

    #[test]
    fn strings() {
        check_lexing(r#""abc" "/*" "*/" "" "'" "#, r#"
Token { kind: String { has_escapes: false }, string: "\"abc\"", line: 1, column: 1 }
Token { kind: BlankSpace, string: " ", line: 1, column: 6 }
Token { kind: String { has_escapes: false }, string: "\"/*\"", line: 1, column: 7 }
Token { kind: BlankSpace, string: " ", line: 1, column: 11 }
Token { kind: String { has_escapes: false }, string: "\"*/\"", line: 1, column: 12 }
Token { kind: BlankSpace, string: " ", line: 1, column: 16 }
Token { kind: String { has_escapes: false }, string: "\"\"", line: 1, column: 17 }
Token { kind: BlankSpace, string: " ", line: 1, column: 19 }
Token { kind: String { has_escapes: false }, string: "\"'\"", line: 1, column: 20 }
Token { kind: BlankSpace, string: " ", line: 1, column: 23 }"#);

        check_lexing(r#""\"""#, r#"
Token { kind: String { has_escapes: true }, string: "\"\\\"\"", line: 1, column: 1 }"#);

        check_lexing(r#""\t\n\r\0\\\"\k""#, r#"
Token { kind: String { has_escapes: true }, string: "\"\\t\\n\\r\\0\\\\\\\"\\k\"", line: 1, column: 1 }"#);
    }


    #[test]
    fn symbols() {
        check_lexing(r#"'134''//''''"'"#, r#"
Token { kind: Symbol { has_escapes: false }, string: "'134'", line: 1, column: 1 }
Token { kind: Symbol { has_escapes: false }, string: "'//'", line: 1, column: 6 }
Token { kind: Symbol { has_escapes: false }, string: "''", line: 1, column: 10 }
Token { kind: Symbol { has_escapes: false }, string: "'\"'", line: 1, column: 12 }"#);

check_lexing(r#"'\'\'\'\\ "'"#, r#"
Token { kind: Symbol { has_escapes: true }, string: "'\\'\\'\\'\\\\ \"'", line: 1, column: 1 }"#);
    }
}
