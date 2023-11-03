use super::context::Context;
use super::node::{ClassDefKind, Node, NodeKind};
use crate::toolchain::diagnostics::diagnostic_emitter::{DiagnosticConsumer, DiagnosticLevel};
use crate::toolchain::diagnostics::diagnostic_kind::{DiagnosticKind, SyntaxDiagnosticKind};
use crate::toolchain::lexer::token::{BinopKind, DelimiterKind, ReservedWordKind, TokenKind};
use crate::toolchain::lexer::tokenized_buffer::TokenizedBuffer;

mod handle_class_def;
mod handle_class_def_body;

// A parse tree.
#[allow(dead_code)] // TODO: remove once we have the next layer in place.
pub struct Tree<'tb> {
    nodes: Vec<Node>,
    tokens: &'tb TokenizedBuffer<'tb>,
    has_error: bool,
}

pub type NodeIndex = usize;

impl<'tb> Tree<'tb> {
    pub fn parse(tokens: &'tb TokenizedBuffer, diags: &mut impl DiagnosticConsumer) -> Tree<'tb> {
        let mut nodes = Vec::new();
        let mut context = Context::new(tokens, &mut nodes, diags);

        context.skip_ignored_tokens();
        context.push_state(NodeKind::TopLevelStatement);

        while let Some(state) = context.state() {
            if context.has_error() {
                break;
            }

            match state.kind {
                // root : topLevelStatement* EOF ;
                // topLevelStatement : classDef
                //                   | classExtension
                //                   | interpreterCode
                //                   ;
                NodeKind::TopLevelStatement => {
                    match context.token_kind() {
                        // Class definitions start with a ClassName token.
                        Some(TokenKind::ClassName) => {
                            context.push_state(NodeKind::ClassDef { kind: ClassDefKind::Root });
                        }
                        // Class Extensions start with a '+' sign.
                        Some(TokenKind::Binop { kind: BinopKind::Plus }) => {
                            context.push_state(NodeKind::ClassExtension);
                        }
                        // Normal expected end of input.
                        None => {
                            context.pop_state();
                        }

                        // Everything else we treat as interpreter input.
                        _ => context.push_state(NodeKind::InterpreterCode),
                    }
                }

                NodeKind::ClassDef { kind: _ } => {
                    handle_class_def::handle_class_def(&mut context);
                }

                NodeKind::ClassDefinitionBody => {
                    handle_class_def_body::handle_class_def_body(&mut context);
                }

                _ => {
                    panic!("unhandled state")
                }
            }
        }

        let has_error = context.has_error();
        Tree { nodes, tokens, has_error }
    }
}

#[cfg(test)]
mod tests {
    use super::{Node, Tree};
    use crate::sclang;
    use crate::toolchain::diagnostics::diagnostic_emitter::NullDiagnosticConsumer;
    use crate::toolchain::lexer::TokenizedBuffer;
    use crate::toolchain::source;

    pub fn check_parsing<'a>(source: &source::SourceBuffer, expect: Vec<Node>) {
        let mut diags = NullDiagnosticConsumer {};
        let tokens = TokenizedBuffer::tokenize(source, &mut diags);
        let tree = Tree::parse(&tokens, &mut diags);
        assert_eq!(tree.nodes, expect);
    }

    #[test]
    fn empty_string() {
        check_parsing(sclang!(""), vec![]);
        check_parsing(sclang!("\n\t\n  "), vec![]);
        check_parsing(sclang!(" /* block comment */"), vec![]);
    }
}
