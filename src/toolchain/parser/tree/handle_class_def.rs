use super::*;

// classDef : CLASSNAME superclass? classDefBody
//          | CLASSNAME SQUARE_OPEN name? SQUARE_CLOSE superclass? CURLY_OPEN classDefBody
//          ;
pub fn handle_class_def(context: &mut Context) {
    debug_assert_eq!(
        context.state().unwrap().kind,
        NodeKind::ClassDef { kind: ClassDefKind::Root }
    );

    // Consume class name
    context.consume_checked(TokenKind::ClassName);

    // Look for optional array storage type declaration, documented within a pair of brackets.
    if context.token_kind() == Some(TokenKind::Delimiter { kind: DelimiterKind::BracketOpen }) {
        // '['
        let open_bracket_index = context.consume();
        let subtree_start = context.tree_size();

        // name?
        if context.token_kind() == Some(TokenKind::Identifier) {
            context.consume_and_add_leaf_node(NodeKind::Name, false);
        }

        let closing_bracket_index = match context.token_kind() {
            Some(TokenKind::Delimiter { kind: DelimiterKind::BracketClose }) => {
                // Nominal case. consume the bracket close.
                Some(context.consume())
            }

            None => {
                let class_token_index = context.state().unwrap().token_index;
                let diag = context
                    .emitter()
                    .build(
                        DiagnosticLevel::Error,
                        DiagnosticKind::SyntaxError {
                            kind: SyntaxDiagnosticKind::UnexpectedEndOfInput,
                        },
                        open_bracket_index,
                        "Unexpected end of input after parsing class array storage type. \
                            Expected closing bracket ']' to match opening bracket here.",
                    )
                    .note(class_token_index, "Class Defined Here.")
                    .emit();
                context.emitter().emit(diag);
                context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
                return;
            }

            // Perhaps we just forgot a closing bracket, emit error, pretend we
            // encountered the closing bracket, and continue.
            _ => {
                let class_token_index = context.state().unwrap().token_index;
                let token_index = context.token_index();
                let diag = context
                    .emitter()
                    .build(
                        DiagnosticLevel::Error,
                        DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                        token_index,
                        "Expected closing bracket ']' in class definition array type.",
                    )
                    .note(class_token_index, "Class defined here.")
                    .note(open_bracket_index, "Opening bracket '[' here.")
                    .emit();
                context.emitter().emit(diag);
                None
            }
        };

        let has_error = closing_bracket_index == None;
        context.add_node(
            NodeKind::ClassDef { kind: ClassDefKind::ArrayStorageType },
            open_bracket_index,
            subtree_start,
            closing_bracket_index,
            has_error,
        )
    }

    // A colon indicates there is a superclass name present.
    // superclass : COLON CLASSNAME ;
    if context.token_kind() == Some(TokenKind::Delimiter { kind: DelimiterKind::Colon }) {
        // ':'
        let colon_index = context.consume();
        let subtree_start = context.tree_size();
        // CLASSNAME
        match context.token_kind() {
            Some(TokenKind::ClassName) => {
                context.consume_and_add_leaf_node(NodeKind::Name, false);
            }

            None => {
                let class_token_index = context.state().unwrap().token_index;
                let diag = context
                    .emitter()
                    .build(
                        DiagnosticLevel::Error,
                        DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnexpectedEndOfInput },
                        colon_index,
                        "Unexpected end of input. Expected a capitalized class name indicating \
                            the name of the superclass after the colon ':' in the class definition.",
                    )
                    .note(
                        class_token_index,
                        "Class defined here.",
                    )
                    .emit();
                context.emitter().emit(diag);
                context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
                return;
            }

            _ => {
                let class_token_index = context.state().unwrap().token_index;
                let token_index = context.token_index();
                let diag = context
                    .emitter()
                    .build(
                        DiagnosticLevel::Error,
                        DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnexpectedToken },
                        token_index,
                        "Unexpected token. Expected a capitalized class name indicating the \
                            name of the superclass after the colon ':' in the class definition.",
                    )
                    .note(colon_index, "Superclass indicator colon ':' here.")
                    .note(class_token_index, "Class defined here.")
                    .emit();
                context.emitter().emit(diag);
                context.consume_and_add_leaf_node(NodeKind::Name, true);
            }
        };

        context.add_node(
            NodeKind::ClassDef { kind: ClassDefKind::Superclass },
            colon_index,
            subtree_start,
            None,
            false,
        );
    }

    // A brace should follow, opening the class definition body.
    match context.token_kind() {
        Some(TokenKind::Delimiter { kind: DelimiterKind::BraceOpen }) => {
            context.push_state(NodeKind::ClassDefinitionBody);
        }

        None => {
            let class_token_index = context.state().unwrap().token_index;
            let diag = context
                .emitter()
                .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError {
                        kind: SyntaxDiagnosticKind::UnexpectedEndOfInput,
                    },
                    class_token_index,
                    "Unexpected end of input while parsing class defined here. Expecting an \
                        open brace '}' to start class definition body.",
                )
                .emit();
            context.emitter().emit(diag);
            context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
            return;
        }

        _ => {
            let class_token_index = context.state().unwrap().token_index;
            let token_index = context.token_index();
            let diag = context
                .emitter()
                .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::MissingToken },
                    token_index,
                    "Unexpected token. Expected opening brace '{' to start class definition \
                        body.",
                )
                .note(class_token_index, "Class defined here.")
                .emit();
            context.emitter().emit(diag);
        }
    };
}

#[cfg(test)]
mod tests {
    use super::super::tests::check_parsing;
    use crate::sclang;
    use crate::toolchain::parser::node::{ClassDefKind, Node, NodeKind::*};
    use crate::toolchain::source;

    #[test]
    fn test_bare_class() {
        check_parsing(
            sclang!("A {} B {}"),
            vec![
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 2,
                    subtree_size: 1,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 0,
                    subtree_size: 2,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 7,
                    subtree_size: 1,
                    closing_token: Some(8),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 5,
                    subtree_size: 2,
                    closing_token: Some(8),
                    has_error: false,
                },
            ],
        );
    }

    #[test]
    fn test_storage_type() {
        check_parsing(
            sclang!("A[float]{}"),
            vec![
                Node { kind: Name, token_index: 2, subtree_size: 1, closing_token: None, has_error: false, },
                Node {
                    kind: ClassDef { kind: ClassDefKind::ArrayStorageType },
                    token_index: 1,
                    subtree_size: 2,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 4,
                    subtree_size: 1,
                    closing_token: Some(5),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 0,
                    subtree_size: 4,
                    closing_token: Some(5),
                    has_error: false,
                },
            ],
        );
    }

    #[test]
    fn test_superclass() {
        check_parsing(
            sclang!("A:B{}"),
            vec![
                Node {
                    kind: Name,
                    token_index: 2,
                    subtree_size: 1,
                    closing_token: None,
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Superclass },
                    token_index: 1,
                    subtree_size: 2,
                    closing_token: None,
                    has_error: false,
                },
                Node {
                    kind: ClassDefinitionBody,
                    token_index: 3,
                    subtree_size: 1,
                    closing_token: Some(4),
                    has_error: false,
                },
                Node {
                    kind: ClassDef { kind: ClassDefKind::Root },
                    token_index: 0,
                    subtree_size: 4,
                    closing_token: Some(4),
                    has_error: false,
                },
            ],
        )
    }
}
