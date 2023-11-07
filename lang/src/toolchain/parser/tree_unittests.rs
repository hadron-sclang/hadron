#[cfg(test)]
pub mod tests {
    use crate::sclang;
    use crate::toolchain::diagnostics::diagnostic_emitter::DiagnosticConsumer;
    use crate::toolchain::diagnostics::DiagnosticKind;
    use crate::toolchain::lexer::TokenizedBuffer;
    use crate::toolchain::parser::{Node, Tree};
    use crate::toolchain::source;

    // We retain these elements from the parser as it produces diagnostics, for comparison against
    // expected
    #[derive(Debug, Eq, PartialEq)]
    pub struct TestDiag {
        pub kind: DiagnosticKind,
        pub line_number: i32,
        pub column_number: i32,
    }

    pub struct ParseTestDiagnosticConsumer {
        pub diags: Vec<TestDiag>,
    }

    impl ParseTestDiagnosticConsumer {
        fn new() -> ParseTestDiagnosticConsumer {
            ParseTestDiagnosticConsumer { diags: Vec::new() }
        }
    }

    impl DiagnosticConsumer for ParseTestDiagnosticConsumer {
        fn handle_diagnostic(
            &mut self,
            diag: crate::toolchain::diagnostics::diagnostic_emitter::Diagnostic,
        ) {
            self.diags.push(TestDiag {
                kind: diag.message.kind,
                line_number: diag.message.location.line_number,
                column_number: diag.message.location.column_number,
            });
        }
        fn flush(&mut self) {}
    }

    pub fn check_parsing(
        source: &source::SourceBuffer,
        expect: Vec<Node>,
        expect_err: Vec<TestDiag>,
    ) {
        let mut diags = ParseTestDiagnosticConsumer::new();
        let tokens = TokenizedBuffer::tokenize(source, &mut diags);
        let tree = Tree::parse(&tokens, &mut diags);
        assert_eq!(tree.nodes(), &expect);
        assert_eq!(diags.diags, expect_err);
    }

    #[test]
    fn empty_string() {
        check_parsing(sclang!(""), vec![], vec![]);
        check_parsing(sclang!("\n\t\n  "), vec![], vec![]);
        check_parsing(sclang!(" /* block comment */"), vec![], vec![]);
    }
}
