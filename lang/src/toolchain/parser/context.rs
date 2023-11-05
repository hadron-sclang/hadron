use crate::toolchain::diagnostics::diagnostic_emitter::{
    DiagnosticBuilder, DiagnosticConsumer, DiagnosticLevel,
};
use crate::toolchain::diagnostics::diagnostic_kind::{DiagnosticKind, SyntaxDiagnosticKind};
use crate::toolchain::lexer::{TokenDiagnosticEmitter, TokenIndex, TokenKind, TokenizedBuffer};
use crate::toolchain::parser::node::{Node, NodeKind};
use crate::toolchain::parser::tree::NodeIndex;

pub struct StateStackEntry {
    pub kind: NodeKind,
    pub subtree_start: NodeIndex,
    pub token_index: TokenIndex,
    // TODO: consider adding has_error here to only propagate the error flag to subtrees?
}

pub struct Context<'tb> {
    states: Vec<StateStackEntry>,
    tokens: &'tb TokenizedBuffer<'tb>,
    nodes: &'tb mut Vec<Node>,
    token_index: TokenIndex,
    emitter: TokenDiagnosticEmitter<'tb, 'tb>,
    has_error: bool,
}

impl<'tb> Context<'tb> {
    pub fn new(
        tokens: &'tb TokenizedBuffer,
        nodes: &'tb mut Vec<Node>,
        diags: &'tb mut impl DiagnosticConsumer,
    ) -> Context<'tb> {
        let states = Vec::new();
        let emitter = TokenDiagnosticEmitter::new(diags, tokens);
        Context { states, tokens, nodes, token_index: 0, emitter, has_error: false }
    }

    pub fn tree_size(&self) -> NodeIndex {
        self.nodes.len()
    }

    pub fn consume_and_add_leaf_node(&mut self, kind: NodeKind, has_error: bool) {
        let token_index = self.consume();
        self.has_error |= has_error;
        self.nodes.push(Node {
            kind,
            token_index,
            subtree_size: 1,
            closing_token: None,
            has_error: self.has_error,
        });
    }

    pub fn add_node(
        &mut self,
        kind: NodeKind,
        token_index: TokenIndex,
        subtree_start: NodeIndex,
        closing_token: Option<TokenIndex>,
        has_error: bool,
    ) {
        self.has_error |= has_error;
        let subtree_size = self.nodes.len() - subtree_start + 1;
        self.nodes.push(Node {
            kind,
            token_index,
            subtree_size,
            closing_token,
            has_error: self.has_error,
        })
    }

    // Pop the current state and use the information to create a new parse node of the given kind,
    // that started when the state was created and ends on the current token.
    pub fn close_state(&mut self, kind: NodeKind, has_error: bool) -> StateStackEntry {
        let state_entry = self.states.pop().unwrap();
        debug_assert_eq!(kind, state_entry.kind);
        self.add_node(
            state_entry.kind,
            state_entry.token_index,
            state_entry.subtree_start,
            Some(self.token_index),
            has_error,
        );
        state_entry
    }

    pub fn push_state(&mut self, kind: NodeKind) {
        self.states.push(StateStackEntry {
            kind,
            subtree_start: self.nodes.len(),
            token_index: self.token_index,
        });
    }

    pub fn state(&self) -> Option<&StateStackEntry> {
        self.states.last()
    }

    /// Returns the [gen]th element up the stack from the current state. In debug mode will assert
    /// if the parent [NodeKind] doesn't match [expect].
    pub fn state_parent(&self, gen: usize, expect: NodeKind) -> Option<&StateStackEntry> {
        let idx = self.states.len() - gen - 1;
        debug_assert_eq!(self.states.get(idx).unwrap().kind, expect);
        self.states.get(idx)
    }

    pub fn pop_state(&mut self) -> Option<StateStackEntry> {
        self.states.pop()
    }

    // Skip any ignored tokens. If the current token is not ignored, do nothing.
    pub fn skip_ignored_tokens(&mut self) {
        while let Some(token) = self.tokens.token_at(self.token_index) {
            match token.kind {
                TokenKind::Ignored { kind: _ } => {
                    self.token_index += 1;
                }
                _ => break,
            }
        }
    }

    pub fn token_index(&self) -> TokenIndex {
        self.token_index
    }

    pub fn last_token(&self) -> TokenIndex {
        self.tokens.tokens().len() - 1
    }

    // Returns the curent token index before advancing to the next non-ignored token.
    pub fn consume(&mut self) -> TokenIndex {
        let index = self.token_index;
        self.token_index += 1;
        self.skip_ignored_tokens();
        index
    }

    // Same as consume but debug_asserts on the current token being the expected kind.
    pub fn consume_checked(&mut self, expected: TokenKind) -> TokenIndex {
        debug_assert!(self.token_kind() == Some(expected));
        self.consume()
    }

    pub fn token_kind(&self) -> Option<TokenKind> {
        Some(self.tokens.token_at(self.token_index)?.kind)
    }

    pub fn emitter(&mut self) -> &mut TokenDiagnosticEmitter<'tb, 'tb> {
        &mut self.emitter
    }

    pub fn has_error(&self) -> bool {
        self.has_error
    }

    pub fn set_error(&mut self) {
        self.has_error = true;
    }

    pub fn unexpected_end_of_input(
        &mut self,
        body: &'static str,
    ) -> DiagnosticBuilder<'tb, TokenIndex> {
        let last_token = self.last_token();
        self.emitter.build(
            DiagnosticLevel::Error,
            DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnexpectedEndOfInput },
            last_token,
            body,
        )
    }

    pub fn unexpected_token(&mut self, body: &'static str) -> DiagnosticBuilder<'tb, TokenIndex> {
        let current_token = self.token_index;
        self.emitter.build(
            DiagnosticLevel::Error,
            DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnexpectedToken },
            current_token,
            body,
        )
    }
}
