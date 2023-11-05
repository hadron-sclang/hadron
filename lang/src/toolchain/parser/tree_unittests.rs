#[cfg(test)]
pub mod tests {
    use crate::sclang;
    use crate::toolchain::diagnostics::diagnostic_emitter::NullDiagnosticConsumer;
    use crate::toolchain::lexer::TokenizedBuffer;
    use crate::toolchain::parser::{Node, Tree};
    use crate::toolchain::source;

    pub fn check_parsing(source: &source::SourceBuffer, expect: Vec<Node>) {
        let mut diags = NullDiagnosticConsumer {};
        let tokens = TokenizedBuffer::tokenize(source, &mut diags);
        let tree = Tree::parse(&tokens, &mut diags);
        assert_eq!(tree.nodes(), &expect);
    }

    #[test]
    fn empty_string() {
        check_parsing(sclang!(""), vec![]);
        check_parsing(sclang!("\n\t\n  "), vec![]);
        check_parsing(sclang!(" /* block comment */"), vec![]);
    }
}
