use super::context::Context;
use super::node::{ClassDefKind, Node, NodeKind};
use crate::toolchain::diagnostics::diagnostic_emitter::{DiagnosticConsumer, DiagnosticLevel};
use crate::toolchain::diagnostics::diagnostic_kind::{DiagnosticKind, SyntaxDiagnosticKind};
use crate::toolchain::lexer::token::{BinopKind, IdentifierKind, DelimiterKind, ReservedKind, TokenKind};
use crate::toolchain::lexer::tokenized_buffer::TokenizedBuffer;

mod handle_class_def;
mod handle_class_def_body;
mod handle_class_ext;
mod handle_class_ext_body;
mod handle_class_var_def;
mod handle_const_def;
mod handle_interpreter_code;
mod handle_member_var_def;
mod handle_member_var_def_list;
mod handle_method_def;
mod handle_name;
mod handle_top_level_statement;

#[cfg(test)]
mod handle_class_def_unittests;

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
                NodeKind::ClassDef { kind: _ } => {
                    handle_class_def::handle_class_def(&mut context);
                }

                NodeKind::ClassDefinitionBody => {
                    handle_class_def_body::handle_class_def_body(&mut context);
                }

                NodeKind::ClassExtension => {
                    handle_class_ext::handle_class_ext(&mut context);
                }

                NodeKind::ClassExtensionBody => {
                    handle_class_ext_body::handle_class_ext_body(&mut context);
                }

                NodeKind::ClassVariableDefinition => {
                    handle_class_var_def::handle_class_var_def(&mut context);
                }

                NodeKind::ConstantDefinition => {
                    handle_const_def::handle_const_def(&mut context);
                }

                NodeKind::InterpreterCode => {
                    handle_interpreter_code::handle_interpreter_code(&mut context);
                }

                NodeKind::MethodDefinition => {
                    handle_method_def::handle_method_def(&mut context);
                }

                NodeKind::MemberVariableDefinition => {
                    handle_member_var_def::handle_member_var_def(&mut context);
                }

                NodeKind::MemberVariableDefinitionList => {
                    handle_member_var_def_list::handle_member_var_def_list(&mut context);
                }

                NodeKind::Name => {
                    handle_name::handle_name(&mut context);
                }

                NodeKind::TopLevelStatement => {
                    handle_top_level_statement::handle_top_level_statement(&mut context);
                }
            }
        }

        let has_error = context.has_error();
        Tree { nodes, tokens, has_error }
    }

    pub fn nodes(&self) -> &Vec<Node> {
        &self.nodes
    }
}
