use crate::toolchain::lexer::tokenized_buffer::TokenizedBuffer;
use crate::toolchain::diagnostics::
use super::node::Node;

// A parse tree.
pub struct Tree<'tb> {
    nodes: Vec<Node>,
    tokens: &'tb TokenizedBuffer<'tb>,
}

impl<'tb> Tree<'tb> {
    pub fn parse(tokens: &'tb TokenizedBuffer, diag: &mut Diag) -> Tree<'tb> {
        let mut nodes = Vec::new();

        Tree { nodes, tokens }
    }
}
