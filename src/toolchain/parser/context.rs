use crate::toolchain::diagnostics::diagnostic_emitter::DiagnosticConsumer;
use crate::toolchain::lexer::tokenized_buffer::{
    TokenDiagnosticEmitter, TokenIndex, TokenizedBuffer,
};
use crate::toolchain::lexer::TokenKind;
use crate::toolchain::parser::node::{Node, NodeKind};
use crate::toolchain::parser::tree::NodeIndex;

#[derive(Clone, Copy, Eq, PartialEq)]
pub enum State {
    // Root-level parsing context, expecting a class definition, extension, or interpreter code.
    TopLevelStatementLoop,

    ClassDefStart,
    ClassDefBody,

    ClassExtStart,

    InterpreterCodeStart,
}

struct StateStackEntry {
    pub state: State,
    pub subtree_start: NodeIndex,
    pub token_index: TokenIndex,
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
    pub fn close_state(&mut self, kind: NodeKind, has_error: bool) {
        let state_entry = self.states.pop().unwrap();
        self.add_node(kind, state_entry.token_index, state_entry.subtree_start, Some(self.token_index), has_error)
    }

    pub fn push_state(&mut self, state: State) {
        self.states.push(StateStackEntry { state, subtree_start: self.nodes.len(), token_index: self.token_index });
    }

    pub fn state(&self) -> Option<State> {
        match self.states.last() {
            Some(entry) => Some(entry.state),
            None => None,
        }
    }

    pub fn pop_state(&mut self) -> Option<State> {
        match self.states.pop() {
            Some(entry) => Some(entry.state),
            None => None,
        }
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

    /// Returns the next non-ignored token kind after the supplied index, if one exists.
    pub fn next_token_after(&self, index: TokenIndex) -> Option<TokenKind> {
        let mut i = index + 1;
        while let Some(token) = self.tokens.token_at(i) {
            match token.kind {
                TokenKind::Ignored { kind: _ } => {
                    i += 1;
                }
                _ => return Some(token.kind),
            }
        }
        None
    }

    // Returns the curent token index before advancing to the next non-ignored token.
    pub fn consume(&mut self) -> TokenIndex {
        let index = self.token_index;
        self.token_index += 1;
        self.skip_ignored_tokens();
        index
    }

    // Advance to next non-ignored token.
    // TODO: should we combine consuming tokens and changing state?
    pub fn next_token(&mut self) {
        self.token_index += 1;
        self.skip_ignored_tokens();
    }

    pub fn token_kind(&self) -> Option<TokenKind> {
        Some(self.tokens.token_at(self.token_index)?.kind)
    }

    pub fn emitter(&mut self) -> &mut TokenDiagnosticEmitter<'tb, 'tb> {
        &mut self.emitter
    }
}
