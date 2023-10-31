use super::*;

// classDef : CLASSNAME superclass? classDefBody
//          | CLASSNAME SQUARE_OPEN name? SQUARE_CLOSE superclass? CURLY_OPEN classDefBody
//          ;
pub fn handle_class_def_start(context: &mut Context) {
    // Consume class name
    debug_assert!(context.token_kind() == Some(TokenKind::ClassName));
    let class_token_index = context.token_index();
    context.consume();

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

            // Perhaps we just forgot a closing bracket, emit error, pretend we
            // encountered the closing bracket, and continue.
            _ => {
                let token_index = context.token_index();
                let diag = context
                    .emitter()
                    .build(
                        DiagnosticLevel::Error,
                        DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                        token_index,
                        "Expected closing bracket ']' in class definition array type.",
                    )
                    .note(
                        DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                        class_token_index,
                        "Class defined here.",
                    )
                    .note(
                        DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                        open_bracket_index,
                        "Opening bracket '[' here.",
                    )
                    .emit();
                context.emitter().emit(diag);
                None
            }
        };

        let has_error = closing_bracket_index != None;
        context.add_node(
            NodeKind::ClassArrayStorageType,
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
            _ => {
                let diag = context.emitter().build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::MissingToken },
                    colon_index,
                    "Expected a capitalized class name indicating the name of the superclass after the colon ':' in the class definition.")
                    .note(DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::MissingToken },
                    class_token_index,
                "Class defined here.")
                    .emit();
                context.emitter().emit(diag);
            }
        };
        context.add_node(NodeKind::ClassSuperclass, colon_index, subtree_start, None, false);
    }

    // A brace should follow, opening the class definition body.
    match context.token_kind() {
        Some(TokenKind::Delimiter { kind: DelimiterKind::BraceOpen }) => {
            context.push_state(State::ClassDefBody);
        }
        _ => {
            let token_index = context.token_index();
            let diag = context
                .emitter()
                .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::MissingToken },
                    token_index, // TODO: might crash if at EOI
                    "Expected opening brace '{' to start class definition body.",
                )
                .note(
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::MissingToken },
                    class_token_index,
                    "Class defined here.",
                )
                .emit();
            context.emitter().emit(diag);
        }
    };
}

#[cfg(test)]
mod tests {
    use super::super::tests::check_parsing;
    use crate::sclang;
    use crate::toolchain::parser::node::{Node, NodeKind};
    use crate::toolchain::source;

    #[test]
    fn test_bare_class() {
        check_parsing(
            sclang!("A {} B {}"),
            vec![
                Node {
                    kind: NodeKind::ClassDefinitionBody,
                    token_index: 2,
                    subtree_size: 1,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: NodeKind::ClassDefinition,
                    token_index: 0,
                    subtree_size: 2,
                    closing_token: Some(3),
                    has_error: false,
                },
                Node {
                    kind: NodeKind::ClassDefinitionBody,
                    token_index: 7,
                    subtree_size: 1,
                    closing_token: Some(8),
                    has_error: false,
                },
                Node {
                    kind: NodeKind::ClassDefinition,
                    token_index: 5,
                    subtree_size: 2,
                    closing_token: Some(8),
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
                    kind: NodeKind::Name,
                    token_index: 2,
                    subtree_size: 1,
                    closing_token: None,
                    has_error: false,
                },
                Node {
                    kind: NodeKind::ClassSuperclass,
                    token_index: 1,
                    subtree_size: 2,
                    closing_token: None,
                    has_error: false,
                },
                Node {
                    kind: NodeKind::ClassDefinitionBody,
                    token_index: 3,
                    subtree_size: 1,
                    closing_token: Some(4),
                    has_error: false,
                },
                Node {
                    kind: NodeKind::ClassDefinition,
                    token_index: 0,
                    subtree_size: 4,
                    closing_token: Some(4),
                    has_error: false,
                },
            ],
        )
    }
}
