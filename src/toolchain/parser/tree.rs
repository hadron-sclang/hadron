use super::context::{Context, State};
use super::node::{Node, NodeKind};
use crate::toolchain::diagnostics::diagnostic_emitter::{DiagnosticConsumer, DiagnosticLevel};
use crate::toolchain::diagnostics::diagnostic_kind::{DiagnosticKind, SyntaxDiagnosticKind};
use crate::toolchain::lexer::token::{BinopKind, DelimiterKind, ReservedWordKind, TokenKind};
use crate::toolchain::lexer::tokenized_buffer::TokenizedBuffer;

mod handle_class_def_body;
mod handle_class_def_start;

// A parse tree.
pub struct Tree<'tb> {
    nodes: Vec<Node>,
    tokens: &'tb TokenizedBuffer<'tb>,
}

pub type NodeIndex = usize;

impl<'tb> Tree<'tb> {
    pub fn parse(tokens: &'tb TokenizedBuffer, diags: &mut impl DiagnosticConsumer) -> Tree<'tb> {
        let mut nodes = Vec::new();
        let mut context = Context::new(tokens, &mut nodes, diags);

        context.skip_ignored_tokens();
        context.push_state(State::TopLevelStatementLoop);

        while let Some(state) = context.state() {
            match state {
                // root : topLevelStatement* EOF ;
                // topLevelStatement : classDef
                //                   | classExtension
                //                   | interpreterCode
                //                   ;
                State::TopLevelStatementLoop => {
                    match context.token_kind() {
                        // Class definitions start with a ClassName token.
                        Some(TokenKind::ClassName) => {
                            context.push_state(State::ClassDefStart);
                        }
                        // Class Extensions start with a '+' sign.
                        Some(TokenKind::Binop { kind: BinopKind::Plus }) => {
                            context.push_state(State::ClassExtStart);
                        }
                        // Normal expected end of input.
                        None => {
                            context.pop_state();
                        }

                        // Everything else we treat as interpreter input.
                        _ => context.push_state(State::InterpreterCodeStart),
                    }
                }

                State::ClassDefStart => {
                    handle_class_def_start::handle_class_def_start(&mut context);
                },

                State::ClassDefBody => {
                    handle_class_def_body::handle_class_def_body(&mut context);
                },

                _ => {
                    panic!("unhandled state")
                }



                /* 

                // classExtension : PLUS CLASSNAME CURLY_OPEN methodDef* CURLY_CLOSE
                //                ;
                State::ClassExtStart => {}

                State::InterpreterCodeStart => {}

                State::ClassDefBody => {}
                */
            }
        }

        Tree { nodes, tokens }
    }
}

#[cfg(test)]
mod tests {
    use crate::sclang;
    use crate::toolchain::diagnostics::diagnostic_emitter::NullDiagnosticConsumer;
    use crate::toolchain::source;
    use crate::toolchain::lexer::TokenizedBuffer;
    use super::{Node, Tree};

    // RPO iterator
    // It's always better when the compiler type-checks your desired input too..
    pub fn check_parsing<'a>(source: &source::SourceBuffer, expect: Vec<Node>) {
        let tokens = TokenizedBuffer::tokenize(source);
        let mut null = NullDiagnosticConsumer {};
        let tree = Tree::parse(&tokens, &mut null);
        assert_eq!(tree.nodes, expect);
    }

    #[test]
    fn empty_string() {
        check_parsing(sclang!(""), vec![]);
        check_parsing(sclang!("\n\t\n  "), vec![]);
        check_parsing(sclang!(" /* block comment */"), vec![]);
    }
}