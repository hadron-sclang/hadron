use super::cursor::Cursor;
use super::Token;
use crate::toolchain::diagnostics::{
    DiagnosticEmitter, DiagnosticLocation, DiagnosticLocationTranslator,
};
use crate::toolchain::source;

pub struct TokenizedBuffer<'s> {
    tokens: Vec<Token<'s>>,
    lines: Vec<&'s str>,
    source: &'s source::SourceBuffer<'s>,
}

pub type TokenIndex = usize;

impl<'s> TokenizedBuffer<'s> {
    pub fn tokenize(source: &'s source::SourceBuffer) -> TokenizedBuffer<'s> {
        let mut lines = Vec::new();
        let cursor = Cursor::new(source.code(), &mut lines);
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

pub type TokenDiagnosticEmitter<'c, 's> = DiagnosticEmitter<'c, 's, TokenIndex>;

#[cfg(test)]
mod tests {
    use crate::sclang;
    use crate::toolchain::source;

    use crate::toolchain::lexer::token::BinopKind::*;
    use crate::toolchain::lexer::token::DelimiterKind::*;
    use crate::toolchain::lexer::token::IgnoredKind::*;
    use crate::toolchain::lexer::token::NumberKind::*;
    use crate::toolchain::lexer::token::ReservedWordKind::*;
    use crate::toolchain::lexer::token::Token;
    use crate::toolchain::lexer::token::TokenKind::*;

    use super::TokenizedBuffer;

    // Lexing helper function to compare expected lexing to a provided debug string of the tokens.
    fn check_lexing<'a>(source: &source::SourceBuffer, expect: Vec<Token<'a>>) {
        let buffer = TokenizedBuffer::tokenize(source);
        assert_eq!(buffer.tokens, expect);
    }

    #[test]
    fn smoke_test() {
        check_lexing(
            sclang!(r#"SynthDef(\a, { var snd = SinOsc.ar(440, mul: 0.5); snd; });"#),
            vec![
                Token { kind: ClassName, string: "SynthDef", line: 1, column: 1 },
                Token { kind: Delimiter { kind: ParenOpen }, string: "(", line: 1, column: 9 },
                Token { kind: InlineSymbol, string: "\\a", line: 1, column: 10 },
                Token { kind: Delimiter { kind: Comma }, string: ",", line: 1, column: 12 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 13 },
                Token { kind: Delimiter { kind: BraceOpen }, string: "{", line: 1, column: 14 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 15 },
                Token { kind: ReservedWord { kind: Var }, string: "var", line: 1, column: 16 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 19 },
                Token { kind: Identifier, string: "snd", line: 1, column: 20 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 23 },
                Token { kind: Binop { kind: Assign }, string: "=", line: 1, column: 24 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 25 },
                Token { kind: ClassName, string: "SinOsc", line: 1, column: 26 },
                Token { kind: Dot, string: ".", line: 1, column: 32 },
                Token { kind: Identifier, string: "ar", line: 1, column: 33 },
                Token { kind: Delimiter { kind: ParenOpen }, string: "(", line: 1, column: 35 },
                Token { kind: Number { kind: Integer }, string: "440", line: 1, column: 36 },
                Token { kind: Delimiter { kind: Comma }, string: ",", line: 1, column: 39 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 40 },
                Token { kind: Keyword, string: "mul:", line: 1, column: 41 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 45 },
                Token { kind: Number { kind: Float }, string: "0.5", line: 1, column: 46 },
                Token { kind: Delimiter { kind: ParenClose }, string: ")", line: 1, column: 49 },
                Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 50 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 51 },
                Token { kind: Identifier, string: "snd", line: 1, column: 52 },
                Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 55 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 56 },
                Token { kind: Delimiter { kind: BraceClose }, string: "}", line: 1, column: 57 },
                Token { kind: Delimiter { kind: ParenClose }, string: ")", line: 1, column: 58 },
                Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 59 },
            ],
        );
    }

    #[test]
    fn binops() {
        // Check the special binops.
        check_lexing(
            sclang!("= * > <- < - | + <>"),
            vec![
                Token { kind: Binop { kind: Assign }, string: "=", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 2 },
                Token { kind: Binop { kind: Asterisk }, string: "*", line: 1, column: 3 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 4 },
                Token { kind: Binop { kind: GreaterThan }, string: ">", line: 1, column: 5 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 6 },
                Token { kind: Binop { kind: LeftArrow }, string: "<-", line: 1, column: 7 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 9 },
                Token { kind: Binop { kind: LessThan }, string: "<", line: 1, column: 10 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 11 },
                Token { kind: Binop { kind: Minus }, string: "-", line: 1, column: 12 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 13 },
                Token { kind: Binop { kind: Pipe }, string: "|", line: 1, column: 14 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 15 },
                Token { kind: Binop { kind: Plus }, string: "+", line: 1, column: 16 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 17 },
                Token { kind: Binop { kind: ReadWriteVar }, string: "<>", line: 1, column: 18 },
            ],
        );

        // Check that the lexer doesn't get confused on binops with special prefixes.
        check_lexing(
            sclang!("== ** >< <-- << -- || ++ <><"),
            vec![
                Token { kind: Binop { kind: BinopIdentifier }, string: "==", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 3 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "**", line: 1, column: 4 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 6 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "><", line: 1, column: 7 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 9 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "<--", line: 1, column: 10 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 13 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "<<", line: 1, column: 14 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 16 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "--", line: 1, column: 17 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 19 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "||", line: 1, column: 20 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 22 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "++", line: 1, column: 23 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 25 },
                Token { kind: Binop { kind: BinopIdentifier }, string: "<><", line: 1, column: 26 },
            ],
        );

        // Comment patterns only lexed as comments at the start of a binop token.
        check_lexing(
            sclang!(r#"+//*"#),
            vec![Token {
                kind: Binop { kind: BinopIdentifier },
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
                Token { kind: Ignored { kind: BlockComment }, string: "/**/", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 5 },
                Token {
                    kind: Ignored { kind: BlockComment },
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
                kind: Ignored { kind: BlockComment },
                string: "/* var a = 2; */",
                line: 1,
                column: 1,
            }],
        );

        // Nested comments.
        check_lexing(
            sclang!(r#"/* / * / * / * /** /**/ // * / * **/*/"#),
            vec![Token {
                kind: Ignored { kind: BlockComment },
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
                Token { kind: Character { is_escaped: true }, string: "$\\t", line: 1, column: 1 },
                Token { kind: Character { is_escaped: false }, string: "$a", line: 1, column: 4 },
                Token { kind: Character { is_escaped: false }, string: "$$", line: 1, column: 6 },
                Token { kind: Character { is_escaped: false }, string: "$ ", line: 1, column: 8 },
                Token { kind: Character { is_escaped: false }, string: "$\"", line: 1, column: 10 },
            ],
        );
    }

    #[test]
    fn class_names() {
        check_lexing(
            sclang!("A B C SinOsc Main_32a"),
            vec![
                Token { kind: ClassName, string: "A", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 2 },
                Token { kind: ClassName, string: "B", line: 1, column: 3 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 4 },
                Token { kind: ClassName, string: "C", line: 1, column: 5 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 6 },
                Token { kind: ClassName, string: "SinOsc", line: 1, column: 7 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 13 },
                Token { kind: ClassName, string: "Main_32a", line: 1, column: 14 },
            ],
        );
    }

    #[test]
    fn delimiters() {
        check_lexing(
            sclang!(r#"^:,(){}[]`#~_;"#),
            vec![
                Token { kind: Delimiter { kind: Caret }, string: "^", line: 1, column: 1 },
                Token { kind: Delimiter { kind: Colon }, string: ":", line: 1, column: 2 },
                Token { kind: Delimiter { kind: Comma }, string: ",", line: 1, column: 3 },
                Token { kind: Delimiter { kind: ParenOpen }, string: "(", line: 1, column: 4 },
                Token { kind: Delimiter { kind: ParenClose }, string: ")", line: 1, column: 5 },
                Token { kind: Delimiter { kind: BraceOpen }, string: "{", line: 1, column: 6 },
                Token { kind: Delimiter { kind: BraceClose }, string: "}", line: 1, column: 7 },
                Token { kind: Delimiter { kind: BracketOpen }, string: "[", line: 1, column: 8 },
                Token { kind: Delimiter { kind: BracketClose }, string: "]", line: 1, column: 9 },
                Token { kind: Delimiter { kind: Grave }, string: "`", line: 1, column: 10 },
                Token { kind: Delimiter { kind: Hash }, string: r"#", line: 1, column: 11 },
                Token { kind: Delimiter { kind: Tilde }, string: "~", line: 1, column: 12 },
                Token { kind: Delimiter { kind: Underscore }, string: "_", line: 1, column: 13 },
                Token { kind: Delimiter { kind: Semicolon }, string: ";", line: 1, column: 14 },
            ],
        );
    }

    #[test]
    fn dots() {
        check_lexing(
            sclang!(". .. ... .... ..... ......"),
            vec![
                Token { kind: Dot, string: ".", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 2 },
                Token { kind: DotDot, string: "..", line: 1, column: 3 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 5 },
                Token { kind: Ellipses, string: "...", line: 1, column: 6 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 9 },
                Token { kind: Ellipses, string: "...", line: 1, column: 10 },
                Token { kind: Dot, string: ".", line: 1, column: 13 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 14 },
                Token { kind: Ellipses, string: "...", line: 1, column: 15 },
                Token { kind: DotDot, string: "..", line: 1, column: 18 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 20 },
                Token { kind: Ellipses, string: "...", line: 1, column: 21 },
                Token { kind: Ellipses, string: "...", line: 1, column: 24 },
            ],
        );
    }

    #[test]
    fn identifiers() {
        check_lexing(
            sclang!("a b c if while const classvar zz_Top3"),
            vec![
                Token { kind: Identifier, string: "a", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 2 },
                Token { kind: Identifier, string: "b", line: 1, column: 3 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 4 },
                Token { kind: Identifier, string: "c", line: 1, column: 5 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 6 },
                Token { kind: Identifier, string: "if", line: 1, column: 7 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 9 },
                Token { kind: Identifier, string: "while", line: 1, column: 10 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 15 },
                Token { kind: ReservedWord { kind: Const }, string: "const", line: 1, column: 16 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 21 },
                Token {
                    kind: ReservedWord { kind: Classvar },
                    string: "classvar",
                    line: 1,
                    column: 22,
                },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 30 },
                Token { kind: Identifier, string: "zz_Top3", line: 1, column: 31 },
            ],
        );

        check_lexing(
            sclang!("mul: add:"),
            vec![
                Token { kind: Keyword, string: "mul:", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 5 },
                Token { kind: Keyword, string: "add:", line: 1, column: 6 },
            ],
        );
    }

    #[test]
    fn inline_symbols() {
        check_lexing(
            sclang!(r#"\a \b \c \1 \ \_ \A_l0ng3r_SYMBOL"#),
            vec![
                Token { kind: InlineSymbol, string: "\\a", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 3 },
                Token { kind: InlineSymbol, string: "\\b", line: 1, column: 4 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 6 },
                Token { kind: InlineSymbol, string: "\\c", line: 1, column: 7 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 9 },
                Token { kind: InlineSymbol, string: "\\1", line: 1, column: 10 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 12 },
                Token { kind: InlineSymbol, string: "\\", line: 1, column: 13 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 14 },
                Token { kind: InlineSymbol, string: "\\_", line: 1, column: 15 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 17 },
                Token { kind: InlineSymbol, string: "\\A_l0ng3r_SYMBOL", line: 1, column: 18 },
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
                    kind: Ignored { kind: LineComment },
                    string: "// start of line\n",
                    line: 1,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: LineComment },
                    string: "// /* start of block\n",
                    line: 2,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: BlankSpace },
                    string: "        ",
                    line: 3,
                    column: 1,
                },
                Token {
                    kind: Ignored { kind: LineComment },
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
                Token { kind: Number { kind: Integer }, string: "123", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 4 },
                Token { kind: Number { kind: IntegerHex }, string: "0x1aAfF", line: 1, column: 5 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 12 },
                Token {
                    kind: Number { kind: IntegerRadix },
                    string: "36rZIGZAG10",
                    line: 1,
                    column: 13,
                },
                Token { kind: Ignored { kind: BlankSpace }, string: "\n", line: 1, column: 24 },
                Token { kind: Number { kind: Float }, string: "100.100", line: 2, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 2, column: 8 },
                Token {
                    kind: Number { kind: FloatAccidental },
                    string: "4bbb",
                    line: 2,
                    column: 9,
                },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 2, column: 13 },
                Token {
                    kind: Number { kind: FloatAccidentalCents },
                    string: "23s200",
                    line: 2,
                    column: 14,
                },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 2, column: 20 },
                Token { kind: Number { kind: FloatRadix }, string: "16rA.F", line: 2, column: 21 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 2, column: 27 },
                Token { kind: Number { kind: FloatSci }, string: "1.7e-9", line: 2, column: 28 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 2, column: 34 },
                Token { kind: Number { kind: FloatSci }, string: "1e+7", line: 2, column: 35 },
            ],
        );
    }

    #[test]
    fn primitives() {
        check_lexing(
            sclang!("_abc123 _Test_ _PrimitiTooTaah"),
            vec![
                Token { kind: Primitive, string: "_abc123", line: 1, column: 1 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 8 },
                Token { kind: Primitive, string: "_Test_", line: 1, column: 9 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 15 },
                Token { kind: Primitive, string: "_PrimitiTooTaah", line: 1, column: 16 },
            ],
        );
    }

    #[test]
    fn strings() {
        check_lexing(
            sclang!(r#""abc" "/*" "*/" "" "'" "#),
            vec![
                Token {
                    kind: String { has_escapes: false },
                    string: "\"abc\"",
                    line: 1,
                    column: 1,
                },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 6 },
                Token { kind: String { has_escapes: false }, string: "\"/*\"", line: 1, column: 7 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 11 },
                Token {
                    kind: String { has_escapes: false },
                    string: "\"*/\"",
                    line: 1,
                    column: 12,
                },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 16 },
                Token { kind: String { has_escapes: false }, string: "\"\"", line: 1, column: 17 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 19 },
                Token { kind: String { has_escapes: false }, string: "\"'\"", line: 1, column: 20 },
                Token { kind: Ignored { kind: BlankSpace }, string: " ", line: 1, column: 23 },
            ],
        );

        check_lexing(
            sclang!(r#""\"""#),
            vec![Token {
                kind: String { has_escapes: true },
                string: "\"\\\"\"",
                line: 1,
                column: 1,
            }],
        );

        check_lexing(
            sclang!(r#""\t\n\r\0\\\"\k""#),
            vec![Token {
                kind: String { has_escapes: true },
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
                Token { kind: Symbol { has_escapes: false }, string: "'134'", line: 1, column: 1 },
                Token { kind: Symbol { has_escapes: false }, string: "'//'", line: 1, column: 6 },
                Token { kind: Symbol { has_escapes: false }, string: "''", line: 1, column: 10 },
                Token { kind: Symbol { has_escapes: false }, string: "'\"'", line: 1, column: 12 },
            ],
        );

        check_lexing(
            sclang!(r#"'\'\'\'\\ "'"#),
            vec![Token {
                kind: Symbol { has_escapes: true },
                string: "'\\'\\'\\'\\\\ \"'",
                line: 1,
                column: 1,
            }],
        );
    }
}
