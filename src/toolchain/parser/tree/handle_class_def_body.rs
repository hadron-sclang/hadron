use mmap_rs::Reserved;

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
    // '{'
    context.consume_checked(TokenKind::Delimiter { kind: DelimiterKind::BraceOpen });

    match context.token_kind() {
        Some(TokenKind::Delimiter { kind: DelimiterKind::BraceClose }) => {
            context.close_state(NodeKind::ClassDefinitionBody, false);
            context.close_state(NodeKind::ClassDefinition, false);
            // '}'
            context.consume();
        }

        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Classvar }) => {
            context.push_state(State::Classvar);
            context.push_state(State::RWDefList);
        }

        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Var }) => {
            context.push_state(State::Var);
            context.push_state(State::RWDefList);
        }

        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Const }) => {
            context.push_state(State::Const);
            context.push_state(State::RWDefList);
        }

        Some(TokenKind::Identifier) | Some(TokenKind::Binop { kind: _ }) => {
            context.push_state(State::MethodDef);
        }

        _ => {
            
        }
    };
}
