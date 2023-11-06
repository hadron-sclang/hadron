#[cfg(test)]
mod tests {
    use crate::sclang;
    use crate::toolchain::diagnostics::diagnostic_emitter;
    use crate::toolchain::source;

    use crate::toolchain::lexer::token::BinopKind;
    use crate::toolchain::lexer::token::DelimiterKind;
    use crate::toolchain::lexer::token::FloatKind;
    use crate::toolchain::lexer::token::IdentifierKind;
    use crate::toolchain::lexer::token::IgnoredKind;
    use crate::toolchain::lexer::token::IntegerKind;
    use crate::toolchain::lexer::token::LiteralKind;
    use crate::toolchain::lexer::token::ReservedKind;
    use crate::toolchain::lexer::token::Token;
    use crate::toolchain::lexer::token::TokenKind::*;

    use crate::toolchain::lexer::TokenizedBuffer;

    // Lexing helper function to compare expected lexing to a provided debug string of the tokens.
    fn check_lexing(source: &source::SourceBuffer, expect: Vec<Token>) {
        let mut diags = diagnostic_emitter::NullDiagnosticConsumer {};
        let buffer = TokenizedBuffer::tokenize(source, &mut diags);
        assert_eq!(buffer.tokens(), &expect);
    }

    #[test]
    fn smoke_test() {
        check_lexing(
            sclang!(r"SynthDef(\a, { var snd = SinOsc.ar(440, mul: 0.5); snd; });"),
            vec![
                Token {
                    kind: Identifier { kind: IdentifierKind::ClassName },
                    string: "SynthDef",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::ParenOpen },
                    string: "(",
                    line: 1,
                    column: 9,
                },
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\a",
                    line: 1,
                    column: 10,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Comma },
                    string: ",",
                    line: 1,
                    column: 12,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 13,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::BraceOpen },
                    string: "{",
                    line: 1,
                    column: 14,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 15,
                },
                Token {
                    kind: Reserved { kind: ReservedKind::Var },
                    string: "var",
                    line: 1,
                    column: 16,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 19,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "snd",
                    line: 1,
                    column: 20,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 23,
                },
                Token { kind: Binop { kind: BinopKind::Assign }, string: "=", line: 1, column: 24 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 25,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::ClassName },
                    string: "SinOsc",
                    line: 1,
                    column: 26,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Dot },
                    string: ".",
                    line: 1,
                    column: 32,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "ar",
                    line: 1,
                    column: 33,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::ParenOpen },
                    string: "(",
                    line: 1,
                    column: 35,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Integer { kind: IntegerKind::Whole } },
                    string: "440",
                    line: 1,
                    column: 36,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Comma },
                    string: ",",
                    line: 1,
                    column: 39,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 40,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Keyword },
                    string: "mul:",
                    line: 1,
                    column: 41,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 45,
                },
                Token {
                    kind: Literal { kind: LiteralKind::FloatingPoint { kind: FloatKind::Simple } },
                    string: "0.5",
                    line: 1,
                    column: 46,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::ParenClose },
                    string: ")",
                    line: 1,
                    column: 49,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Semicolon },
                    string: ";",
                    line: 1,
                    column: 50,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 51,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "snd",
                    line: 1,
                    column: 52,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Semicolon },
                    string: ";",
                    line: 1,
                    column: 55,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 56,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::BraceClose },
                    string: "}",
                    line: 1,
                    column: 57,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::ParenClose },
                    string: ")",
                    line: 1,
                    column: 58,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Semicolon },
                    string: ";",
                    line: 1,
                    column: 59,
                },
            ],
        );
    }

    #[test]
    fn binops() {
        // Check the special binops.
        check_lexing(
            sclang!("= * > <- < - | + <>"),
            vec![
                Token { kind: Binop { kind: BinopKind::Assign }, string: "=", line: 1, column: 1 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 2,
                },
                Token {
                    kind: Binop { kind: BinopKind::Asterisk },
                    string: "*",
                    line: 1,
                    column: 3,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 4,
                },
                Token {
                    kind: Binop { kind: BinopKind::GreaterThan },
                    string: ">",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Binop { kind: BinopKind::LeftArrow },
                    string: "<-",
                    line: 1,
                    column: 7,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 9,
                },
                Token {
                    kind: Binop { kind: BinopKind::LessThan },
                    string: "<",
                    line: 1,
                    column: 10,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 11,
                },
                Token { kind: Binop { kind: BinopKind::Minus }, string: "-", line: 1, column: 12 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 13,
                },
                Token { kind: Binop { kind: BinopKind::Pipe }, string: "|", line: 1, column: 14 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 15,
                },
                Token { kind: Binop { kind: BinopKind::Plus }, string: "+", line: 1, column: 16 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 17,
                },
                Token {
                    kind: Binop { kind: BinopKind::ReadWriteVar },
                    string: "<>",
                    line: 1,
                    column: 18,
                },
            ],
        );

        // Check that the lexer doesn't get confused on binops with special prefixes.
        check_lexing(
            sclang!("== ** >< <-- << -- || ++ <><"),
            vec![
                Token { kind: Binop { kind: BinopKind::Name }, string: "==", line: 1, column: 1 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 3,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "**", line: 1, column: 4 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 6,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "><", line: 1, column: 7 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 9,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "<--", line: 1, column: 10 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 13,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "<<", line: 1, column: 14 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 16,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "--", line: 1, column: 17 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 19,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "||", line: 1, column: 20 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 22,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "++", line: 1, column: 23 },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 25,
                },
                Token { kind: Binop { kind: BinopKind::Name }, string: "<><", line: 1, column: 26 },
            ],
        );

        // Comment patterns only lexed as comments at the start of a binop token.
        check_lexing(
            sclang!(r#"+//*"#),
            vec![Token {
                kind: Binop { kind: BinopKind::Name },
                string: "+//*",
                line: 1,
                column: 1,
            }],
        );
    }

    #[test]
    fn block_comments() {
        // Simple inline comments.
        check_lexing(
            sclang!("/**/ /*/ hello */"),
            vec![
                Token {
                    kind: Ignored { kind: IgnoredKind::BlockComment },
                    string: "/**/",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlockComment },
                    string: "/*/ hello */",
                    line: 1,
                    column: 6,
                },
            ],
        );

        // Commented out code.
        check_lexing(
            sclang!(r#"/* var a = 2; */"#),
            vec![Token {
                kind: Ignored { kind: IgnoredKind::BlockComment },
                string: "/* var a = 2; */",
                line: 1,
                column: 1,
            }],
        );

        // Nested comments.
        check_lexing(
            sclang!(r#"/* / * / * / * /** /**/ // * / * **/*/"#),
            vec![Token {
                kind: Ignored { kind: IgnoredKind::BlockComment },
                string: "/* / * / * / * /** /**/ // * / * **/*/",
                line: 1,
                column: 1,
            }],
        );
    }

    #[test]
    fn characters() {
        check_lexing(
            sclang!(r#"$\t$a$$$ $""#),
            vec![
                Token {
                    kind: Literal { kind: LiteralKind::Character { is_escaped: true } },
                    string: "$\\t",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Character { is_escaped: false } },
                    string: "$a",
                    line: 1,
                    column: 4,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Character { is_escaped: false } },
                    string: "$$",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Character { is_escaped: false } },
                    string: "$ ",
                    line: 1,
                    column: 8,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Character { is_escaped: false } },
                    string: "$\"",
                    line: 1,
                    column: 10,
                },
            ],
        );
    }

    #[test]
    fn class_names() {
        check_lexing(
            sclang!("A B C SinOsc Main_32a"),
            vec![
                Token {
                    kind: Identifier { kind: IdentifierKind::ClassName },
                    string: "A",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 2,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::ClassName },
                    string: "B",
                    line: 1,
                    column: 3,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 4,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::ClassName },
                    string: "C",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::ClassName },
                    string: "SinOsc",
                    line: 1,
                    column: 7,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 13,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::ClassName },
                    string: "Main_32a",
                    line: 1,
                    column: 14,
                },
            ],
        );
    }

    #[test]
    fn delimiters() {
        check_lexing(
            sclang!(r#"^:,(){}[]`#~_;"#),
            vec![
                Token {
                    kind: Delimiter { kind: DelimiterKind::Caret },
                    string: "^",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Colon },
                    string: ":",
                    line: 1,
                    column: 2,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Comma },
                    string: ",",
                    line: 1,
                    column: 3,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::ParenOpen },
                    string: "(",
                    line: 1,
                    column: 4,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::ParenClose },
                    string: ")",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::BraceOpen },
                    string: "{",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::BraceClose },
                    string: "}",
                    line: 1,
                    column: 7,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::BracketOpen },
                    string: "[",
                    line: 1,
                    column: 8,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::BracketClose },
                    string: "]",
                    line: 1,
                    column: 9,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Grave },
                    string: "`",
                    line: 1,
                    column: 10,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Hash },
                    string: r"#",
                    line: 1,
                    column: 11,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Tilde },
                    string: "~",
                    line: 1,
                    column: 12,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Underscore },
                    string: "_",
                    line: 1,
                    column: 13,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Semicolon },
                    string: ";",
                    line: 1,
                    column: 14,
                },
            ],
        );
    }

    #[test]
    fn dots() {
        check_lexing(
            sclang!(". .. ... .... ..... ......"),
            vec![
                Token {
                    kind: Delimiter { kind: DelimiterKind::Dot },
                    string: ".",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 2,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::DotDot },
                    string: "..",
                    line: 1,
                    column: 3,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Ellipses },
                    string: "...",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 9,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Ellipses },
                    string: "...",
                    line: 1,
                    column: 10,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Dot },
                    string: ".",
                    line: 1,
                    column: 13,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 14,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Ellipses },
                    string: "...",
                    line: 1,
                    column: 15,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::DotDot },
                    string: "..",
                    line: 1,
                    column: 18,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 20,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Ellipses },
                    string: "...",
                    line: 1,
                    column: 21,
                },
                Token {
                    kind: Delimiter { kind: DelimiterKind::Ellipses },
                    string: "...",
                    line: 1,
                    column: 24,
                },
            ],
        );
    }

    #[test]
    fn identifiers() {
        check_lexing(
            sclang!("a b c if while const classvar zz_Top3"),
            vec![
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "a",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 2,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "b",
                    line: 1,
                    column: 3,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 4,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "c",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "if",
                    line: 1,
                    column: 7,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 9,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "while",
                    line: 1,
                    column: 10,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 15,
                },
                Token {
                    kind: Reserved { kind: ReservedKind::Const },
                    string: "const",
                    line: 1,
                    column: 16,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 21,
                },
                Token {
                    kind: Reserved { kind: ReservedKind::Classvar },
                    string: "classvar",
                    line: 1,
                    column: 22,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 30,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Name },
                    string: "zz_Top3",
                    line: 1,
                    column: 31,
                },
            ],
        );

        check_lexing(
            sclang!("mul: add:"),
            vec![
                Token {
                    kind: Identifier { kind: IdentifierKind::Keyword },
                    string: "mul:",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Keyword },
                    string: "add:",
                    line: 1,
                    column: 6,
                },
            ],
        );
    }

    #[test]
    fn inline_symbols() {
        check_lexing(
            sclang!(r"\a \b \c \1 \ \_ \A_l0ng3r_SYMBOL"),
            vec![
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\a",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 3,
                },
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\b",
                    line: 1,
                    column: 4,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\c",
                    line: 1,
                    column: 7,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 9,
                },
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\1",
                    line: 1,
                    column: 10,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 12,
                },
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\",
                    line: 1,
                    column: 13,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 14,
                },
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\_",
                    line: 1,
                    column: 15,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 17,
                },
                Token {
                    kind: Literal { kind: LiteralKind::InlineSymbol },
                    string: "\\A_l0ng3r_SYMBOL",
                    line: 1,
                    column: 18,
                },
            ],
        );
    }

    #[test]
    fn line_comments() {
        check_lexing(
            sclang!(
                r#"// start of line
// /* start of block
        // end of line */"#
            ),
            vec![
                Token {
                    kind: Ignored { kind: IgnoredKind::LineComment },
                    string: "// start of line\n",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::LineComment },
                    string: "// /* start of block\n",
                    line: 2,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: "        ",
                    line: 3,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::LineComment },
                    string: "// end of line */",
                    line: 3,
                    column: 9,
                },
            ],
        );
    }

    #[test]
    fn numbers() {
        check_lexing(
            sclang!(
                r#"123 0x1aAfF 36rZIGZAG10
100.100 4bbb 23s200 16rA.F 1.7e-9 1e+7"#
            ),
            vec![
                Token {
                    kind: Literal { kind: LiteralKind::Integer { kind: IntegerKind::Whole } },
                    string: "123",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 4,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Integer { kind: IntegerKind::Hexadecimal } },
                    string: "0x1aAfF",
                    line: 1,
                    column: 5,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 12,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Integer { kind: IntegerKind::Radix } },
                    string: "36rZIGZAG10",
                    line: 1,
                    column: 13,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: "\n",
                    line: 1,
                    column: 24,
                },
                Token {
                    kind: Literal { kind: LiteralKind::FloatingPoint { kind: FloatKind::Simple } },
                    string: "100.100",
                    line: 2,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 2,
                    column: 8,
                },
                Token {
                    kind: Literal {
                        kind: LiteralKind::FloatingPoint { kind: FloatKind::Accidental },
                    },
                    string: "4bbb",
                    line: 2,
                    column: 9,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 2,
                    column: 13,
                },
                Token {
                    kind: Literal { kind: LiteralKind::FloatingPoint { kind: FloatKind::Cents } },
                    string: "23s200",
                    line: 2,
                    column: 14,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 2,
                    column: 20,
                },
                Token {
                    kind: Literal { kind: LiteralKind::FloatingPoint { kind: FloatKind::Radix } },
                    string: "16rA.F",
                    line: 2,
                    column: 21,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 2,
                    column: 27,
                },
                Token {
                    kind: Literal {
                        kind: LiteralKind::FloatingPoint { kind: FloatKind::Scientific },
                    },
                    string: "1.7e-9",
                    line: 2,
                    column: 28,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 2,
                    column: 34,
                },
                Token {
                    kind: Literal {
                        kind: LiteralKind::FloatingPoint { kind: FloatKind::Scientific },
                    },
                    string: "1e+7",
                    line: 2,
                    column: 35,
                },
            ],
        );
    }

    #[test]
    fn primitives() {
        check_lexing(
            sclang!("_abc123 _Test_ _PrimitiTooTaah"),
            vec![
                Token {
                    kind: Identifier { kind: IdentifierKind::Primitive },
                    string: "_abc123",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 8,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Primitive },
                    string: "_Test_",
                    line: 1,
                    column: 9,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 15,
                },
                Token {
                    kind: Identifier { kind: IdentifierKind::Primitive },
                    string: "_PrimitiTooTaah",
                    line: 1,
                    column: 16,
                },
            ],
        );
    }

    #[test]
    fn strings() {
        check_lexing(
            sclang!(r#""abc" "/*" "*/" "" "'" "#),
            vec![
                Token {
                    kind: Literal { kind: LiteralKind::String { has_escapes: false } },
                    string: "\"abc\"",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Literal { kind: LiteralKind::String { has_escapes: false } },
                    string: "\"/*\"",
                    line: 1,
                    column: 7,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 11,
                },
                Token {
                    kind: Literal { kind: LiteralKind::String { has_escapes: false } },
                    string: "\"*/\"",
                    line: 1,
                    column: 12,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 16,
                },
                Token {
                    kind: Literal { kind: LiteralKind::String { has_escapes: false } },
                    string: "\"\"",
                    line: 1,
                    column: 17,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 19,
                },
                Token {
                    kind: Literal { kind: LiteralKind::String { has_escapes: false } },
                    string: "\"'\"",
                    line: 1,
                    column: 20,
                },
                Token {
                    kind: Ignored { kind: IgnoredKind::BlankSpace },
                    string: " ",
                    line: 1,
                    column: 23,
                },
            ],
        );

        check_lexing(
            sclang!(r#""\"""#),
            vec![Token {
                kind: Literal { kind: LiteralKind::String { has_escapes: true } },
                string: "\"\\\"\"",
                line: 1,
                column: 1,
            }],
        );

        check_lexing(
            sclang!(r#""\t\n\r\0\\\"\k""#),
            vec![Token {
                kind: Literal { kind: LiteralKind::String { has_escapes: true } },
                string: "\"\\t\\n\\r\\0\\\\\\\"\\k\"",
                line: 1,
                column: 1,
            }],
        );
    }

    #[test]
    fn symbols() {
        check_lexing(
            sclang!(r#"'134''//''''"'"#),
            vec![
                Token {
                    kind: Literal { kind: LiteralKind::Symbol { has_escapes: false } },
                    string: "'134'",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Symbol { has_escapes: false } },
                    string: "'//'",
                    line: 1,
                    column: 6,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Symbol { has_escapes: false } },
                    string: "''",
                    line: 1,
                    column: 10,
                },
                Token {
                    kind: Literal { kind: LiteralKind::Symbol { has_escapes: false } },
                    string: "'\"'",
                    line: 1,
                    column: 12,
                },
            ],
        );

        check_lexing(
            sclang!(r#"'\'\'\'\\ "'"#),
            vec![Token {
                kind: Literal { kind: LiteralKind::Symbol { has_escapes: true } },
                string: "'\\'\\'\\'\\\\ \"'",
                line: 1,
                column: 1,
            }],
        );
    }
}
