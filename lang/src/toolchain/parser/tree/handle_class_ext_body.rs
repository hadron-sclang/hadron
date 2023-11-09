use super::*;

// classExtension  : PLUS CLASSNAME CURLY_OPEN methodDef* CURLY_CLOSE
//                 ;
//
// methodDef : ASTERISK? methodDefName CURLY_OPEN argDecls? varDecls? primitive? body? CURLY_CLOSE ;
//
pub fn handle_class_ext_body(context: &mut Context) {
    debug_assert_eq!(context.state().unwrap().kind, NodeKind::ClassExtensionBody);

    // '{'
    context.consume_checked(TokenKind::Group { kind: GroupKind::BraceOpen });

    expect!(context,
        TokenKind::Group { kind: GroupKind::BraceClose }, {
            context.close_state(NodeKind::ClassExtensionBody, false);
            context.close_state(NodeKind::ClassExtension, false);
            // '}'
            context.consume();
        },
        TokenKind::Identifier { kind: IdentifierKind::Name }
        | TokenKind::Binop { kind: _ }, {
            // Name or binop. Binop could be class method '*' or name of method.
            context.push_state(NodeKind::MethodDefinition);
        }
    );
}
