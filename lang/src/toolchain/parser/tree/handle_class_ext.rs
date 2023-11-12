use super::*;

// classExtension  : PLUS CLASSNAME CURLY_OPEN methodDef* CURLY_CLOSE
//                 ;
pub fn handle_class_ext(context: &mut Context) {
    debug_assert_eq!(context.state().unwrap().kind, NodeKind::ClassExtension);

    // '+'
    context.consume_checked(TokenKind::Binop { kind: BinopKind::Plus });

    // extended classname should follow '+'
    expect!(context, TokenKind::Identifier { kind: IdentifierKind::ClassName }, {
        context.consume_and_add_leaf_node(NodeKind::Name, false);
    });

    // Open brace '{' should follow classname.
    expect!(context, TokenKind::Group { kind: GroupKind::BraceOpen }, {
        context.push_state(NodeKind::ClassExtensionBody);
    });
}
