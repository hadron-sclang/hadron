use super::*;

// classDef : CLASSNAME superclass? CURLY_OPEN classVarDecl* methodDef* CURLY_CLOSE
//          | CLASSNAME SQUARE_OPEN name? SQUARE_CLOSE superclass? CURLY_OPEN
//              classVarDecl* methodDef* CURLY_CLOSE
//          ;
pub fn handle_class_def_start(context: &mut Context) {
    // Consume class name
    debug_assert!(context.token_kind() == Some(TokenKind::ClassName));

    let state = context.pop_state();
    debug_assert!(state == Some(State::ClassDefStart));

    let class_token_index = context.token_index();
    context.consume_and_add_leaf_node(NodeKind::ClassName, false);

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
            },

            // Perhaps we just forgot a closing bracket, emit error, pretend we
            // encountered the closing bracket, and continue.
            _ => {
                let token_index = context.token_index();
                let diag = context.emitter().build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError{ kind: SyntaxDiagnosticKind::UnclosedPair },
                    token_index,
                    "Expected closing bracket ']' in class definition array type.")
                    .note(
                        DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                        class_token_index,
                        "Class defined here.")
                    .note(
                        DiagnosticKind::SyntaxError{ kind: SyntaxDiagnosticKind::UnclosedPair },
                        open_bracket_index,
                        "Opening bracket '[' here.")
                    .emit();
                context.emitter().emit(diag);
                None
            }
        };

        let has_error = closing_bracket_index != None;
        context.add_node(NodeKind::ClassArrayStorageType, open_bracket_index,
            subtree_start, closing_bracket_index, has_error)
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
                context.consume_and_add_leaf_node(NodeKind::ClassName, false);
            },
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
        context.add_node(NodeKind::Superclass, colon_index, subtree_start, None, false);
    }

    // A brace should follow, opening the class definition body.
    match context.token_kind() {
        Some(TokenKind::Delimiter { kind: DelimiterKind::BraceOpen }) => {
            context.push_state(State::ClassDefBody);
        },
        _ => {
            let token_index = context.token_index();
            let diag = context.emitter().build(
                DiagnosticLevel::Error,
                DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::MissingToken },
                token_index, // TODO: might crash if at EOI
                "Expected opening brace '{' to start class definition body.")
            .note(DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::MissingToken },
            class_token_index,
                "Class defined here.")
            .emit();
            context.emitter().emit(diag);
        }
    };
}