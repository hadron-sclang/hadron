use super::*;

// classDefBody : CURLY_OPEN classVarDecl* methodDef* CURLY_CLOSE
//              ;
//
// classVarDecl : CLASSVAR rwSlotDefList SEMICOLON
//              | VAR rwSlotDefList SEMICOLON
//              | CONST constDefList SEMICOLON
//              ;
//
// methodDef : ASTERISK? methodDefName CURLY_OPEN argDecls? varDecls? primitive? body? CURLY_CLOSE ;
//
// methodDefName : name
//               | binop
//               ;
//
pub fn handle_class_def_body(context: &mut Context) {
    debug_assert!(context.state() == Some(State::ClassDefBody));
    debug_assert!(
        context.token_kind() == Some(TokenKind::Delimiter { kind: DelimiterKind::BraceOpen })
    );
    // '{'
    context.consume();

    match context.token_kind() {
        Some(TokenKind::Delimiter { kind: DelimiterKind::BraceClose }) => {
            context.close_state(NodeKind::ClassDefinitionBody, false);
            context.close_state(NodeKind::ClassDefinition, false);
            // '}'
            context.consume();
        }

        _ => {}
    };
}
