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
/// a [Token]. [Token]s are as small as possible to be cache-friendly.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Token {
    /// The kind of Token.
    pub kind: TokenKind,

    /// Token length in *characters*, not bytes.
    pub length: u32,
}

impl Token {
    fn new(kind: TokenKind, length: u32) -> Token {
        Token { kind, length }
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

    /// A character literal, such as `$a` or `$\t`. `is_escaped` is `false` in the former example,
    /// `true` in the latter.
    Character { is_escaped: bool },

    /// A class name starting with an uppercase letter, such as `SynthDef`.
    ClassName,

    /// `:` single-character delimiter.
    Colon,

    /// `,` single-character delimiter.
    Comma,

    /// A single period.
    Dot,

    /// Two periods.
    DotDot,

    /// Three periods.
    Ellipses,

    /// This internal [TokenKind] is for signaling the end of the stream to the iterator. It should
    /// not appear to [Token] consumers.
    EndOfInput,

    /// \` single-character delimiter.
    Grave,

    /// `#` single-character delimiter.
    Hash,

    /// Any valid name, including reserved words like `var`.
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

    /// `)` single-character delimiter.
    ParenClose,

    /// `(` single-character delimiter.
    ParenOpen,

    /// A binding to a C++ function in legacy SuperCollider. An underscore followed by one or more
    /// valid identifier characters (alphanumeric or underscore). For example, `_Basic_New`.
    Primitive,

    /// `;` single-character delimiter.
    Semicolon,

    /// A double-quoted character literal sequence. If it has backslash (`\`) escape characters in
    /// it `has_escapes` is true, telling later stages if they must process the string more or can
    /// copy it directly to the string literal.
    String { has_escapes: bool },

    /// A single-quoted character literal sequence.
    Symbol { has_escapes: bool },

    /// `~` single-character delimiter.
    Tilde,

    /// `_` single-character delimiter.
    Underscore,

    /// Anything that the lexer didn't recognize as valid SuperCollider language input.
    Unknown
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

/// Returns a [Token] iterator over the `input` string.
pub fn tokenize(input: &str) -> impl Iterator<Item = Token> + '_ {
    let mut cursor = Cursor::new(input);
    std::iter::from_fn(move || {
        let token = cursor.advance_token();
        if token.kind != TokenKind::EndOfInput { Some(token) } else { None }
    })
}

impl Cursor<'_> {
    pub fn advance_token(&mut self) -> Token {
        let first_char = match self.bump() {
            Some(c) => c,
            None => return Token::new(TokenKind::EndOfInput, 0),
        };

        let token_kind = match first_char {
            // Either a block or line comment.
            '/' => match self.first() {
                // Double slashes mean a line comment.
                '/' => self.line_comment(),
                '*' => self.block_comment(),
                _ => TokenKind::Unknown,
            },

            // Blank spaces.
            c if is_blank_space(c) => self.blank_space(),

            // Identifiers starting with a lowercase letter
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
            '^' => TokenKind::Caret,
            ':' => TokenKind::Colon,
            ',' => TokenKind::Comma,
            '(' => TokenKind::ParenOpen,
            ')' => TokenKind::ParenClose,
            '{' => TokenKind::BraceOpen,
            '}' => TokenKind::BraceClose,
            '[' => TokenKind::BracketOpen,
            ']' => TokenKind::BracketClose,
            '`' => TokenKind::Grave,
            '#' => TokenKind::Hash,
            '~' => TokenKind::Tilde,
            ';' => TokenKind::Semicolon,

            // Underscores can be single-character delimiters, but if they are followed by an
            // alphanumeric string or more underscores, we lex them as primitives.
            '_' => {
                if is_identifier(self.first()) {
                    self.eat_while(|c| is_identifier(c));
                    TokenKind::Primitive
                } else {
                    TokenKind::Underscore
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
            c if is_binop(c) => {
                // Most special meaning binop identifiers are only a single character.
                let binop_kind = self.binop_kind(c);
                TokenKind::Binop { kind: binop_kind }
            },

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

        let token = Token::new(token_kind, self.counter());
        self.reset_counter();
        token
    }

    fn line_comment(&mut self) -> TokenKind {
        // Consume the second forward slash in the block comment beginner.
        self.bump();
        self.eat_while(|c| c != '\n');
        // Consume the line end character.
        self.bump();
        TokenKind::LineComment
    }

    fn block_comment(&mut self) -> TokenKind {
        // Consume the star in the block comment
        self.bump();

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

    fn binop_kind(&mut self, c: char) -> BinopKind {
        // Most interesting binops are a single character in length. Check for those first.
        if !is_binop(self.first()) {
            return match c {
                '*' => BinopKind::Asterisk,
                '=' => BinopKind::Assign,
                '>' => BinopKind::GreaterThan,
                '<' => BinopKind::LessThan,
                '-' => BinopKind::Minus,
                '|' => BinopKind::Pipe,
                '+' => BinopKind::Plus,
                _ => BinopKind::Identifier,
            };
        }

        // Both of the two-character interesting binops start with '<'.
        if c == '<' {
            let next = self.bump().expect("Expected a valid binop character after '<'");
            if !is_binop(self.first()) {
                return match next {
                    '-' => BinopKind::LeftArrow,
                    '>' => BinopKind::ReadWriteVar,
                    _ => BinopKind::Identifier
                }
            }
        }

        self.eat_while(|c| is_binop(c));
        BinopKind::Identifier
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

    fn check_lexing(src: &str, expect: &str) {
        let actual: String = tokenize(src).map(|token| format!("\n{:?}", token)).collect();
        assert_eq!(expect, &actual);
    }

    #[test]
    fn smoke_test() {
        check_lexing(r#"SynthDef(\a, { var snd = SinOsc.ar(440, mul: 0.5); snd; });"#, r#"
Token { kind: ClassName, length: 8 }
Token { kind: ParenOpen, length: 1 }
Token { kind: InlineSymbol, length: 2 }
Token { kind: Comma, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: BraceOpen, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 3 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 3 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Assign }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: ClassName, length: 6 }
Token { kind: Dot, length: 1 }
Token { kind: Identifier, length: 2 }
Token { kind: ParenOpen, length: 1 }
Token { kind: Number { kind: Integer }, length: 3 }
Token { kind: Comma, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Keyword, length: 4 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: Float }, length: 3 }
Token { kind: ParenClose, length: 1 }
Token { kind: Semicolon, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 3 }
Token { kind: Semicolon, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: BraceClose, length: 1 }
Token { kind: ParenClose, length: 1 }
Token { kind: Semicolon, length: 1 }"#);
    }

    #[test]
    fn binops() {
        // Check the special binops.
        check_lexing("= * > <- < - | + <>", r#"
Token { kind: Binop { kind: Assign }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Asterisk }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: GreaterThan }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: LeftArrow }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: LessThan }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Minus }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Pipe }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Plus }, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: ReadWriteVar }, length: 2 }"#);

        // Check that the lexer doesn't get confused on binops with special prefixes.
        check_lexing("== ** >< <-- << -- || ++ <><", r#"
Token { kind: Binop { kind: Identifier }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 3 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Binop { kind: Identifier }, length: 3 }"#);
    }

    #[test]
    fn block_comments() {
        // Simple inline comments.
        check_lexing("/**/ /*/ hello */", r#"
Token { kind: BlockComment, length: 4 }
Token { kind: BlankSpace, length: 1 }
Token { kind: BlockComment, length: 12 }"#);

        // Multi-line comment.
        check_lexing(r#"/*
*********
//        Some Documentation
    Some Code: var a = 2; SinOsc.ar(a);
       */"#, r#"
Token { kind: BlockComment, length: 91 }"#);

        // Nested comments.
        check_lexing(r#"/* / * / * / *
    /************
        /**/
     / * / * *****/
*/"#, r#"
Token { kind: BlockComment, length: 68 }"#);
    }

    #[test]
    fn delimiters() {
        check_lexing(r#"^:,(){}[]`#~_;"#, r#"
Token { kind: Caret, length: 1 }
Token { kind: Colon, length: 1 }
Token { kind: Comma, length: 1 }
Token { kind: ParenOpen, length: 1 }
Token { kind: ParenClose, length: 1 }
Token { kind: BraceOpen, length: 1 }
Token { kind: BraceClose, length: 1 }
Token { kind: BracketOpen, length: 1 }
Token { kind: BracketClose, length: 1 }
Token { kind: Grave, length: 1 }
Token { kind: Hash, length: 1 }
Token { kind: Tilde, length: 1 }
Token { kind: Underscore, length: 1 }
Token { kind: Semicolon, length: 1 }"#);
    }

    #[test]
    fn characters() {
        check_lexing(r#"$\t$a$$$ $""#, r#"
Token { kind: Character { is_escaped: true }, length: 3 }
Token { kind: Character { is_escaped: false }, length: 2 }
Token { kind: Character { is_escaped: false }, length: 2 }
Token { kind: Character { is_escaped: false }, length: 2 }
Token { kind: Character { is_escaped: false }, length: 2 }"#);
    }

    #[test]
    fn class_names() {
        check_lexing("A B C SinOsc Main_32a", r#"
Token { kind: ClassName, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: ClassName, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: ClassName, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: ClassName, length: 6 }
Token { kind: BlankSpace, length: 1 }
Token { kind: ClassName, length: 8 }"#);
    }

    #[test]
    fn dots() {
        check_lexing(". .. ... .... ..... ......", r#"
Token { kind: Dot, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: DotDot, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Ellipses, length: 3 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Ellipses, length: 3 }
Token { kind: Dot, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Ellipses, length: 3 }
Token { kind: DotDot, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Ellipses, length: 3 }
Token { kind: Ellipses, length: 3 }"#);
    }

    #[test]
    fn identifiers() {
        check_lexing("a b c if while const classvar zz_Top3", r#"
Token { kind: Identifier, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 5 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 5 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 8 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Identifier, length: 7 }"#);

        check_lexing("mul: add:",r#"
Token { kind: Keyword, length: 4 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Keyword, length: 4 }"#);
}

    #[test]
    fn inline_symbols() {
        check_lexing(r#"\a \b \c \1 \ \_ \A_l0ng3r_SYMBOL"#, r#"
Token { kind: InlineSymbol, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: InlineSymbol, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: InlineSymbol, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: InlineSymbol, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: InlineSymbol, length: 1 }
Token { kind: BlankSpace, length: 1 }
Token { kind: InlineSymbol, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: InlineSymbol, length: 16 }"#);
    }

    #[test]
    fn line_comments() {
        check_lexing(r#"// start of line
    // end of line
// /* start of block */"#, r#"
Token { kind: LineComment, length: 17 }
Token { kind: BlankSpace, length: 4 }
Token { kind: LineComment, length: 15 }
Token { kind: LineComment, length: 23 }"#);
    }

    #[test]
    fn numbers() {
        check_lexing(r#"123 0x1aAfF 36rZIGZAG10
100.100 4bbb 23s200 16rA.F 1.7e-9 1e+7"#, r#"
Token { kind: Number { kind: Integer }, length: 3 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: IntegerHex }, length: 7 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: IntegerRadix }, length: 11 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: Float }, length: 7 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: FloatAccidental }, length: 4 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: FloatAccidentalCents }, length: 6 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: FloatRadix }, length: 6 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: FloatSci }, length: 6 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Number { kind: FloatSci }, length: 4 }"#);
    }

    #[test]
    fn primitives() {
        check_lexing("_abc123 _Test_ _PrimitiTooTaah", r#"
Token { kind: Primitive, length: 7 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Primitive, length: 6 }
Token { kind: BlankSpace, length: 1 }
Token { kind: Primitive, length: 15 }"#);
    }

    #[test]
    fn strings() {
        check_lexing(r#""abc" "/*" "*/" "" "'" "#, r#"
Token { kind: String { has_escapes: false }, length: 5 }
Token { kind: BlankSpace, length: 1 }
Token { kind: String { has_escapes: false }, length: 4 }
Token { kind: BlankSpace, length: 1 }
Token { kind: String { has_escapes: false }, length: 4 }
Token { kind: BlankSpace, length: 1 }
Token { kind: String { has_escapes: false }, length: 2 }
Token { kind: BlankSpace, length: 1 }
Token { kind: String { has_escapes: false }, length: 3 }
Token { kind: BlankSpace, length: 1 }"#);

        check_lexing(r#""\t\n\r\0\\\"\k""#, r#"
Token { kind: String { has_escapes: true }, length: 16 }"#);
    }

    #[test]
    fn symbols() {
        check_lexing(r#"'134''//''''"'"#, r#"
Token { kind: Symbol { has_escapes: false }, length: 5 }
Token { kind: Symbol { has_escapes: false }, length: 4 }
Token { kind: Symbol { has_escapes: false }, length: 2 }
Token { kind: Symbol { has_escapes: false }, length: 3 }"#);

        check_lexing(r#"'\'\'\'\\ "'"#, r#"
Token { kind: Symbol { has_escapes: true }, length: 12 }"#);
    }
}
