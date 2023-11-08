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
    context.consume_checked(TokenKind::Identifier { kind: IdentifierKind::ClassName });

    // Look for optional array storage type declaration, documented within a pair of brackets.
    if context.token_kind() == Some(TokenKind::Group { kind: GroupKind::BracketOpen }) {
        // '['
        let open_bracket_index = context.consume();
        let subtree_start = context.tree_size();

        // name?
        if context.token_kind() == Some(TokenKind::Identifier { kind: IdentifierKind::Name }) {
            context.consume_and_add_leaf_node(NodeKind::Name, false);
        }

        close_inline_group!(context, TokenKind::Group { kind: GroupKind::BracketClose }, {
            let closing_bracket_index = context.token_index();
            context.consume_checked(TokenKind::Group { kind: GroupKind::BracketClose });
            context.add_node(
                NodeKind::ClassDef { kind: ClassDefKind::ArrayStorageType },
                open_bracket_index,
                subtree_start,
                Some(closing_bracket_index),
                false,
            )
        });
    }

    // A colon indicates there is a superclass name present.
    // superclass : COLON CLASSNAME ;
    if context.token_kind() == Some(TokenKind::Delimiter { kind: DelimiterKind::Colon }) {
        // ':'
        let colon_index = context.consume();
        let subtree_start = context.tree_size();

        expect!(context, TokenKind::Identifier { kind: IdentifierKind::ClassName }, {
            context.consume_and_add_leaf_node(NodeKind::Name, false);
        });

        context.add_node(
            NodeKind::ClassDef { kind: ClassDefKind::Superclass },
            colon_index,
            subtree_start,
            None,
            false,
        );
    }

    // A brace should follow, opening the class definition body.
    expect!(context, TokenKind::Group { kind: GroupKind::BraceOpen }, {
        context.push_state(NodeKind::ClassDefinitionBody);
    });
}
