use crate::toolchain::diagnostics::diagnostic_emitter::DiagnosticConsumer;

use crate::toolchain::lexer::token::{BinopKind, DelimiterKind, FloatKind, LiteralKind};
use crate::toolchain::lexer::{TokenDiagnosticEmitter, TokenIndex, TokenKind, TokenizedBuffer};
use crate::toolchain::parser::node::{Node, NodeKind};
use crate::toolchain::parser::tree::NodeIndex;

pub struct StateStackEntry {
    pub kind: NodeKind,
    pub subtree_start: NodeIndex,
    pub token_index: TokenIndex,
    // TODO: consider adding has_error here to only propagate the error flag to subtrees?
}

#[allow(dead_code)]
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

    pub fn add_leaf_node(
        &mut self,
        kind: NodeKind,
        token_index: TokenIndex,
        closing_token: Option<TokenIndex>,
        has_error: bool,
    ) {
        self.has_error |= has_error;
        self.nodes.push(Node {
            kind,
            token_index,
            subtree_size: 1,
            closing_token,
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

    pub fn has_error(&self) -> bool {
        self.has_error
    }

    pub fn recover_unexpected_token(&mut self) {
        // Issue error.
        // Find closing tokens for the pushed states.
    }

    pub fn recover_unexpected_end_of_input(&mut self) {
        // Issue error.
        // Blow state stack.
    }

    // literal : coreLiteral
    //         | list
    //         ;

    // coreLiteral : integer
    //             | floatingPoint
    //             | strings
    //             | symbol
    //             | booleanConstant
    //             | CHARACTER
    //             | NIL
    //             ;

    // integer : integerNumber
    //         | MINUS integerNumber
    //         ;

    // integerNumber : INT
    //               | INT_HEX
    //               | INT_RADIX
    //               ;

    // floatingPoint : floatLiteral
    //               | floatLiteral PI
    //               | integer PI
    //               | PI
    //               | MINUS PI
    //               | accidental
    //               ;

    // floatLiteral : floatNumber
    //              | MINUS floatNumber
    //              ;

    // floatNumber : FLOAT
    //             | FLOAT_RADIX
    //             | FLOAT_SCI
    //             | INF
    //             ;

    // accidental : FLOAT_FLAT
    //            | FLOAT_FLAT_CENTS
    //            | FLOAT_SHARP
    //            | FLOAT_SHARP_CENTS
    //            ;

    // strings : STRING+ ;

    // symbol : SYMBOL_QUOTE
    //        | SYMBOL_SLASH
    //        ;

    // booleanConstant : TRUE
    //                 | FALSE
    //                 ;

    // list : HASH innerListLiteral ;

    // innerListLiteral : SQUARE_OPEN listLiterals? SQUARE_CLOSE
    //                  | CLASSNAME SQUARE_OPEN listLiterals? SQUARE_CLOSE
    //                  ;

    // listLiteral : coreLiteral
    //             | innerListLiteral
    //             | innerDictLiteral
    //             | name
    //             ;

    // listLiterals : listLiteral (COMMA listLiteral)* ;

    // innerDictLiteral : PAREN_OPEN dictLiterals? PAREN_CLOSE ;

    // dictLiterals : dictLiteral (COMMA dictLiteral)* COMMA? ;

    // dictLiteral : listLiteral COLON listLiteral
    //             | KEYWORD listLiteral
    //             ;

    /// Consumes the current and all subsequent tokens that parse as a literal. Returns true if
    /// any tokens were consumed.
    pub fn consume_literal(&mut self) -> bool {
        let starting_token_index = self.token_index;
        let tk = self.token_kind();
        if tk.is_none() {
            return false;
        }
        let tk = tk.unwrap();

        match tk {
            // Strings supporting automatic concatencation.
            TokenKind::Literal { kind: LiteralKind::String { has_escapes: _ } } => {
                self.consume();
                while self.token_kind()
                    == Some(TokenKind::Literal { kind: LiteralKind::String { has_escapes: false } })
                    || self.token_kind()
                        == Some(TokenKind::Literal {
                            kind: LiteralKind::String { has_escapes: true },
                        })
                {
                    self.consume();
                }
                self.add_leaf_node(
                    NodeKind::Literal,
                    starting_token_index,
                    Some(self.token_index - 1),
                    false,
                );
                true
            }

            // Numeric floats and integers may be followed by a PI, check for that.
            TokenKind::Literal { kind: LiteralKind::FloatingPoint { kind: FloatKind::Inf } }
            | TokenKind::Literal { kind: LiteralKind::FloatingPoint { kind: FloatKind::Radix } }
            | TokenKind::Literal {
                kind: LiteralKind::FloatingPoint { kind: FloatKind::Scientific },
            }
            | TokenKind::Literal { kind: LiteralKind::FloatingPoint { kind: FloatKind::Simple } }
            | TokenKind::Literal { kind: LiteralKind::Integer { kind: _ } } => {
                self.consume();
                if self.token_kind()
                    == Some(TokenKind::Literal {
                        kind: LiteralKind::FloatingPoint { kind: FloatKind::Pi },
                    })
                {
                    self.consume();
                }
                self.add_leaf_node(
                    NodeKind::Literal,
                    starting_token_index,
                    Some(self.token_index - 1),
                    false,
                );
                true
            }

            // Floats and Ints can have unary negation in front of them, so look ahead one token
            // and if it's a numeric literal the two tokens together make the full literal.
            TokenKind::Binop { kind: BinopKind::Minus } => {
                // Temporarily consume the minus sign to get to the next token.
                self.consume();
                let next = self.token_kind();
                match next {
                    Some(TokenKind::Literal { kind: LiteralKind::Integer { kind: _ } })
                    | Some(TokenKind::Literal { kind: LiteralKind::FloatingPoint { kind: _ } }) => {
                        self.consume();
                        // Numeric literals can still have a 'pi' suffix, look for it
                        match next {
                            // TODO: this is a repeat of the un-negated PI matching. Refactor.
                            Some(TokenKind::Literal {
                                kind: LiteralKind::FloatingPoint { kind: FloatKind::Inf },
                            })
                            | Some(TokenKind::Literal {
                                kind: LiteralKind::FloatingPoint { kind: FloatKind::Radix },
                            })
                            | Some(TokenKind::Literal {
                                kind: LiteralKind::FloatingPoint { kind: FloatKind::Scientific },
                            })
                            | Some(TokenKind::Literal {
                                kind: LiteralKind::FloatingPoint { kind: FloatKind::Simple },
                            })
                            | Some(TokenKind::Literal { kind: LiteralKind::Integer { kind: _ } }) => {
                                if self.token_kind()
                                    == Some(TokenKind::Literal {
                                        kind: LiteralKind::FloatingPoint { kind: FloatKind::Pi },
                                    })
                                {
                                    self.consume();
                                }
                            }
                            _ => { /* do nothing */ }
                        };
                        self.add_leaf_node(
                            NodeKind::Literal,
                            starting_token_index,
                            Some(self.token_index - 1),
                            false,
                        );
                        true
                    }

                    _ => {
                        // reset token index to hyphen, this isn't a literal
                        self.token_index = starting_token_index;
                        false
                    }
                }
            }

            // TODO: List and Dict literals
            TokenKind::Delimiter { kind: DelimiterKind::Hash } => {
                panic!("List literals not supported yet.");
            }

            TokenKind::Literal { kind: _ } => {
                self.consume_and_add_leaf_node(NodeKind::Literal, false);
                true
            }

            _ => false,
        }
    }
}
