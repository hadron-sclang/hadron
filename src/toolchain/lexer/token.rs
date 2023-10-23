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
        Token {
            kind,
            string,
            line,
            column,
        }
    }

    pub fn end() -> Token<'a> {
        Token {
            kind: TokenKind::EndOfInput,
            string: "",
            line: 0,
            column: 0,
        }
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

    /// A character literal, such as `$a` or `$\t`. `is_escaped` is `false` in the former example,
    /// `true` in the latter.
    Character {
        is_escaped: bool,
    },

    /// A class name starting with an uppercase letter, such as `SynthDef`.
    ClassName,

    Delimiter {
        kind: DelimiterKind,
    },

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

    /// The compiler ignores any empty space or comments.
    Ignored {
        kind: IgnoredKind,
    },

    /// A symbol starting with a forward slash, such as `\synth`.
    InlineSymbol,

    /// An identifier followed by a colon, `add:` for example.
    Keyword,

    /// A numeric literal, either floating-point or integer. The [NumberKind] gives a hint about
    /// how to convert it to machine representation.
    Number {
        kind: NumberKind,
    },

    /// A binding to a C++ function in legacy SuperCollider. An underscore followed by one or more
    /// valid identifier characters (alphanumeric or underscore). For example, `_Basic_New`.
    Primitive,

    ReservedWord {
        kind: ReservedWordKind,
    },

    /// A double-quoted character literal sequence. If it has backslash (`\`) escape characters in
    /// it `has_escapes` is true, telling later stages if they must process the string more or can
    /// copy it directly to the string literal.
    String {
        has_escapes: bool,
    },

    /// A single-quoted character literal sequence.
    Symbol {
        has_escapes: bool,
    },

    /// Anything that the lexer didn't recognize as valid SuperCollider language input.
    Unknown,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BinopKind {
    /// The `=` operator.
    Assign,

    /// The `*` operator.
    Asterisk,

    /// A non-specific binop that didn't match the reserved binop patterns, `+/+` for example.
    BinopIdentifier,

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
pub enum IgnoredKind {
    /// Blank space, including some extended characters like the vertical tab. See
    /// `is_blank_space()` for details.
    BlankSpace,

    /// A block comment, including any nested comments within. It may contain line breaks.
    BlockComment,

    /// A double-slash comment terminated by the end of the line or input.
    LineComment,
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
    Var,
}
