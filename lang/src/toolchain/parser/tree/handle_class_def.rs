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

            // Perhaps user just forgot a closing bracket, emit error, pretend we
            // encountered the closing bracket, and continue.
            Some(_) => {
                let class_token_index = context.state().unwrap().token_index;
                let diag = context
                    .unexpected_token(
                        "Expected closing bracket ']' in class definition array type.",
                    )
                    .note(class_token_index, "Class defined here.")
                    .note(open_bracket_index, "Opening bracket '[' here.")
                    .emit();
                context.emitter().emit(diag);
                None
            }

            // Unexpected end of input.
            None => {
                let class_token_index = context.state().unwrap().token_index;
                let diag = context
                    .unexpected_end_of_input(
                        "Unexpected end of input after parsing class array storage type. \
                            Expected closing bracket ']' to match opening bracket here.",
                    )
                    .note(class_token_index, "Class Defined Here.")
                    .note(open_bracket_index, "Opening bracket '[' here.")
                    .emit();
                context.emitter().emit(diag);
                context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
                return;
            }
        };

        let has_error = closing_bracket_index.is_none();
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

            Some(_) => {
                let class_token_index = context.state().unwrap().token_index;
                let diag = context
                    .unexpected_token(
                        "Unexpected token. Expected a capitalized class name indicating the \
                            name of the superclass after the colon ':' in the class definition.",
                    )
                    .note(colon_index, "Superclass indicator colon ':' here.")
                    .note(class_token_index, "Class defined here.")
                    .emit();
                context.emitter().emit(diag);
                context.consume_and_add_leaf_node(NodeKind::Name, true);
            }

            None => {
                let class_token_index = context.state().unwrap().token_index;
                let diag = context
                    .unexpected_end_of_input(
                        "Unexpected end of input. Expected a capitalized class name indicating \
                            the name of the superclass after the colon ':' in the class definition.",
                    )
                    .note(class_token_index,"Class defined here.",
                    )
                    .emit();
                context.emitter().emit(diag);
                context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
                return;
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

        Some(_) => {
            let class_token_index = context.state().unwrap().token_index;
            let diag = context
                .unexpected_token(
                    "Unexpected token. Expected opening brace '{' to start class definition \
                        body.",
                )
                .note(class_token_index, "Class defined here.")
                .emit();
            context.emitter().emit(diag);
        }

        None => {
            let class_token_index = context.state().unwrap().token_index;
            let diag = context
                .unexpected_end_of_input(
                    "Unexpected end of input while parsing class defined here. Expecting an \
                        open brace '}' to start class definition body.",
                )
                .note(class_token_index, "Class defined here.")
                .emit();
            context.emitter().emit(diag);
            context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
        }
    };
}
