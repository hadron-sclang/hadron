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

        None => {}

        _ => {
            let token_index = context.token_index();
            let diag = context
                .emitter()
                .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnexpectedToken },
                    token_index,
                    "Unexpected token in class definition body. Starting tokens expected \
                    inside a class definition body are 'classvar', 'var', 'const', and method \
                    definitions.",
                )
                .note(
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnexpectedToken },
                    0,
                    "Class definition body opened here.",
                )
                .note(
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnexpectedToken },
                    0,
                    "Class defined here.",
                )
                .emit();
            context.emitter().emit(diag);
        }
    };
}
