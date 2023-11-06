use super::*;

// classExtension  : PLUS CLASSNAME CURLY_OPEN methodDef* CURLY_CLOSE
//                 ;
pub fn handle_class_ext(context: &mut Context) {
    debug_assert_eq!(context.state().unwrap().kind, NodeKind::ClassExtension);

    // '+'
    context.consume_checked(TokenKind::Binop { kind: BinopKind::Plus });

    // '+' signifier should be followed by a classname.
    match context.token_kind() {
        Some(TokenKind::Identifier { kind: IdentifierKind::ClassName }) => {
            context.consume_and_add_leaf_node(NodeKind::Name, false);
        }

        // Unexpected token.
        Some(_) => {
            let plus_index = context.state().unwrap().token_index;
            let diag = context
                .unexpected_token(
                    "Unexpected token while parsing class extension. Expected \
                        capitalized class name to follow class extension plus sign '+'.",
                )
                .note(plus_index, "Class extension started here.".to_string())
                .emit();
            context.emitter().emit(diag);
        }

        // Unexpected end of input.
        None => {
            let diag = context
                .unexpected_end_of_input(
                    "Unexpected end of input after parsing class extension plus sign '+'. \
                        Expecting capitalized class name after plus sign, indicating the class to \
                        extend.",
                )
                .emit();
            context.emitter().emit(diag);
            context.close_state(NodeKind::ClassExtension, true);
            return;
        }
    }

    // Open brace '{' should follow classname.
    match context.token_kind() {
        Some(TokenKind::Group { kind: GroupKind::BraceOpen }) => {
            context.push_state(NodeKind::ClassExtensionBody);
        }

        // Unexpected token, emit error and continue.
        Some(_) => {
            let plus_index = context.state().unwrap().token_index;
            let diag = context
                .unexpected_token(
                    "Unexpected token after parsing class extension class name. \
                        Expecting brace open '{' to follow.",
                )
                .note(plus_index, "Class extension started here.".to_string())
                .emit();
            context.emitter().emit(diag);
            context.close_state(NodeKind::ClassExtension, true);
        }

        // Unexpected end of input.
        None => {
            let diag = context
                .unexpected_end_of_input(
                    "Unexpected end of input after parsing \
                        class name. Expecting brace open '{' to follow.",
                )
                .emit();
            context.emitter().emit(diag);
            context.close_state(NodeKind::ClassExtension, true);
        }
    }
}
