use std::fmt::Display;

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
    pub line: i32,

    // The 1-based character column on the line.
    pub column: i32,
}

impl<'a> Token<'a> {
    pub fn new(kind: TokenKind, string: &'a str, line: i32, column: i32) -> Token<'a> {
        Token { kind, string, line, column }
    }

    pub fn end() -> Token<'a> {
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
    Binop {
        kind: BinopKind,
    },

    Delimiter {
        kind: DelimiterKind,
    },

    /// This internal [TokenKind] is for signaling the end of the stream to the iterator. It should
    /// not appear to [Token] consumers.
    EndOfInput,

    /// Any name starting with a lowercase letter and followed by 0 or more alphanumeric letters or
    /// underscore `_`.
    Identifier {
        kind: IdentifierKind,
    },

    /// The compiler ignores any empty space or comments.
    Ignored {
        kind: IgnoredKind,
    },

    /// A literal value in the code.
    Literal {
        kind: LiteralKind,
    },

    Reserved {
        kind: ReservedKind,
    },
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BinopKind {
    /// The `=` operator.
    Assign,

    /// The `*` operator.
    Asterisk,

    /// The `>` operator.
    GreaterThan,

    /// The `<-` operator.
    LeftArrow,

    /// The `<` operator.
    LessThan,

    /// The `-` operator.
    Minus,

    /// A non-specific binop that didn't match the reserved binop patterns, `+/+` for example.
    Name,

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

    /// A single period.
    Dot,

    /// Two periods.
    DotDot,

    /// Three periods.
    Ellipses,

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
pub enum IgnoredKind {
    /// Blank space, including some extended characters like the vertical tab. See
    /// `is_blank_space()` for details.
    BlankSpace,

    /// A block comment, including any nested comments within. It may contain line breaks.
    BlockComment,

    /// Invalid utf-8 sequence in input string, will terminate lexing.
    Invalid,

    /// A double-slash comment terminated by the end of the line or input.
    LineComment,

    /// Anything that the lexer didn't recognize as valid SuperCollider language input.
    Unknown,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IdentifierKind {
    /// A class name starting with an uppercase letter, such as `SynthDef`.
    ClassName,

    /// A name followed by a colon, `add:` for example.
    Keyword,

    /// A name starting with a lowercase letter, such as 'pants'.
    Name,

    /// A binding to a C++ function in legacy SuperCollider. An underscore followed by one or more
    /// valid identifier characters (alphanumeric or underscore). For example, `_Basic_New`.
    Primitive,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LiteralKind {
    /// A boolean literal, either 'true' or 'false'.
    Boolean { value: bool },

    /// A character literal, such as `$a` or `$\t`. `is_escaped` is `false` in the former example,
    /// `true` in the latter.
    Character { is_escaped: bool },

    /// A floating point numeric literal.
    FloatingPoint { kind: FloatKind },

    /// A symbol starting with a forward slash, such as `\synth`.
    InlineSymbol,

    /// An integer numeric literal.
    Integer { kind: IntegerKind },

    /// `nil` is the constant literal empty type.
    Nil,

    /// A double-quoted character literal sequence. If it has backslash (`\`) escape characters in
    /// it `has_escapes` is true, telling later stages if they must process the string more or can
    /// copy it directly to the string literal.
    String { has_escapes: bool },

    /// A single-quoted character literal sequence.
    Symbol { has_escapes: bool },
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IntegerKind {
    /// A base-10 integer number, `0` for example.
    Whole,

    /// A base-10 number followed by an `x` and then a base-16 integer number, `0xdeadbeef` for
    /// example.
    Hexadecimal,

    /// A base-10 number followed by an `r` and then a radix integer number, `2r100` for example.
    Radix,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum FloatKind {
    /// A base-10 number followed by 1-3 `b` or `s` characters, `10bb` for example.
    Accidental,

    /// A base-10 number followed by a `b` or `s` character and a cent number, `10b499` for
    /// example.
    Cents,

    /// `inf` is a constant float value representing infinity.
    Inf,

    /// `pi` is a constant float value representing pi.
    Pi,

    /// A base-10 radix number followed by an `r` and then a radix floating point number,
    /// `36rHADR0N.C0LLID3R` for example.
    Radix,

    /// A base-10 number with a single dot, `1.0` for example.
    Simple,

    /// A base-10 radix floating point number (with optional dot) followed by an `e` and signed
    /// exponent number.
    Scientific,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ReservedKind {
    /// `arg` declares method and function arguments.
    Arg,

    /// `classvar` marks a variable as belonging to the class.
    Classvar,

    /// `const` marks a class member as read-only.
    Const,

    /// `var` declares local and object instance variables.
    Var,
}

impl Display for TokenKind {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        let s = match self {
            TokenKind::Binop { kind: BinopKind::Assign } => "equals '='",
            TokenKind::Binop { kind: BinopKind::Asterisk } => "asterisk '*'",
            TokenKind::Binop { kind: BinopKind::GreaterThan } => "greater than '>'",
            TokenKind::Binop { kind: BinopKind::LeftArrow } => "left arrow '<-'",
            TokenKind::Binop { kind: BinopKind::LessThan } => "less than '<'",
            TokenKind::Binop { kind: BinopKind::Minus } => "minus '-'",
            TokenKind::Binop { kind: BinopKind::Name } => "binary operator",
            TokenKind::Binop { kind: BinopKind::Pipe } => "pipe '|'",
            TokenKind::Binop { kind: BinopKind::Plus } => "plus '+'",
            TokenKind::Binop { kind: BinopKind::ReadWriteVar } => "read/write sign '<>'",

            TokenKind::Delimiter { kind: DelimiterKind::BraceClose } => "closing brace '}'",
            TokenKind::Delimiter { kind: DelimiterKind::BraceOpen } => "opening brace '{'",
            TokenKind::Delimiter { kind: DelimiterKind::BracketClose } => "closing bracket ']'",
            TokenKind::Delimiter { kind: DelimiterKind::BracketOpen } => "opening bracket '['",
            TokenKind::Delimiter { kind: DelimiterKind::Caret } => "caret '^'",
            TokenKind::Delimiter { kind: DelimiterKind::Colon } => "colon ':'",
            TokenKind::Delimiter { kind: DelimiterKind::Comma } => "comma ','",
            TokenKind::Delimiter { kind: DelimiterKind::Dot } => "dot '.'",
            TokenKind::Delimiter { kind: DelimiterKind::DotDot } => "double dot '..'",
            TokenKind::Delimiter { kind: DelimiterKind::Ellipses } => "ellipses '...'",
            TokenKind::Delimiter { kind: DelimiterKind::Grave } => "grave '^'",
            TokenKind::Delimiter { kind: DelimiterKind::Hash } => "hash mark '$'",
            TokenKind::Delimiter { kind: DelimiterKind::ParenClose } => "closing parenthesis ')'",
            TokenKind::Delimiter { kind: DelimiterKind::ParenOpen } => "opening parenthesis '('",
            TokenKind::Delimiter { kind: DelimiterKind::Semicolon } => "semicolon ':'",
            TokenKind::Delimiter { kind: DelimiterKind::Tilde } => "tilde '~'",
            TokenKind::Delimiter { kind: DelimiterKind::Underscore } => "underscore '_'",

            TokenKind::EndOfInput => {
                panic!("EndOfInput should never appear to token consumers")
            }

            TokenKind::Identifier { kind: IdentifierKind::ClassName } => "class name",
            TokenKind::Identifier { kind: IdentifierKind::Keyword } => "keyword",
            TokenKind::Identifier { kind: IdentifierKind::Name } => "name",
            TokenKind::Identifier { kind: IdentifierKind::Primitive } => "primitive",

            TokenKind::Ignored { kind: IgnoredKind::BlankSpace } => "blank space",
            TokenKind::Ignored { kind: IgnoredKind::BlockComment } => "block comment",
            TokenKind::Ignored { kind: IgnoredKind::Invalid } => "invalid",
            TokenKind::Ignored { kind: IgnoredKind::LineComment } => "line comment",
            TokenKind::Ignored { kind: IgnoredKind::Unknown } => "unknown",

            TokenKind::Literal { kind: LiteralKind::Boolean { value: _ } } => "boolean literal",
            TokenKind::Literal { kind: LiteralKind::Character { is_escaped: _ } } => {
                "character literal"
            }
            TokenKind::Literal { kind: LiteralKind::FloatingPoint { kind: _ } } => {
                "floating point literal"
            }
            TokenKind::Literal { kind: LiteralKind::InlineSymbol } => "inline symbol literal",
            TokenKind::Literal { kind: LiteralKind::Integer { kind: _ } } => "integer literal",
            TokenKind::Literal { kind: LiteralKind::Nil } => "nil literal",
            TokenKind::Literal { kind: LiteralKind::String { has_escapes: _ } } => "string literal",
            TokenKind::Literal { kind: LiteralKind::Symbol { has_escapes: _ } } => "symbol literal",

            TokenKind::Reserved { kind: ReservedKind::Arg } => "reserved word 'arg'",
            TokenKind::Reserved { kind: ReservedKind::Classvar } => "reserved word 'classvar'",
            TokenKind::Reserved { kind: ReservedKind::Const } => "reserved word 'const'",
            TokenKind::Reserved { kind: ReservedKind::Var } => "reserved word 'var'",
        };
        f.write_str(s)
    }
}

impl<'s> Display for Token<'s> {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        match self.kind {
            TokenKind::Binop { kind: BinopKind::Name }
            | TokenKind::Identifier { kind: _ }
            | TokenKind::Ignored { kind: IgnoredKind::Unknown } => {
                f.write_fmt(format_args!("{} '{}'", self.kind, self.string))
            }

            // Many tokens are text-invariant, so repeating the token text is redundant.
            TokenKind::Binop { kind: _ }
            | TokenKind::Delimiter { kind: _ }
            | TokenKind::EndOfInput
            | TokenKind::Ignored { kind: _ }
            | TokenKind::Literal { kind: LiteralKind::Nil }
            | TokenKind::Reserved { kind: _ } => f.write_fmt(format_args!("{}", self.kind)),

            // TODO: probably a smarter way to collapse thiis back into the printing kinds up top.
            TokenKind::Literal { kind: _ } => {
                f.write_fmt(format_args!("{} '{}'", self.kind, self.string))
            }
        }
    }
}
