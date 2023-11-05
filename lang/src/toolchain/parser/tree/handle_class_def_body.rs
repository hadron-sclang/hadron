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
    debug_assert_eq!(context.state().unwrap().kind, NodeKind::ClassDefinitionBody);

    // '{'
    context.consume_checked(TokenKind::Delimiter { kind: DelimiterKind::BraceOpen });

    match context.token_kind() {
        // '}' means end of class definition.
        Some(TokenKind::Delimiter { kind: DelimiterKind::BraceClose }) => {
            context.close_state(NodeKind::ClassDefinitionBody, false);
            context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, false);
            // '}'
            context.consume();
        }

        // 'classvar'
        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Classvar }) => {
            context.push_state(NodeKind::ClassVariableDefinition);
            context.push_state(NodeKind::MemberVariableDefinitionList);
        }

        // 'var'
        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Var }) => {
            context.push_state(NodeKind::MemberVariableDefinition);
            context.push_state(NodeKind::MemberVariableDefinitionList);
        }

        // 'const'
        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Const }) => {
            context.push_state(NodeKind::ConstantDefinition);
        }

        // Identifier or binop. Binop could be class method '*' or name of method.
        Some(TokenKind::Identifier) | Some(TokenKind::Binop { kind: _ }) => {
            context.push_state(NodeKind::MethodDefinition);
        }

        // Unexpected token.
        Some(_) => {
            let token_index = context.state().unwrap().token_index;
            let class_body =
                context.state_parent(1, NodeKind::ClassDefinitionBody).unwrap().token_index;
            let class_def = context
                .state_parent(2, NodeKind::ClassDef { kind: ClassDefKind::Root })
                .unwrap()
                .token_index;
            let diag = context
                  .emitter()
                  .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                    token_index,
                    "Unexpected token in class definition body. Starting tokens expected \
                    inside a class definition body are 'classvar', 'var', 'const', and method \
                    definitions. Did you forget a closing brace '}'?",
                )
                .note(class_body, "Class definition body opened here.")
                .note(class_def, "Class defined here.")
                .emit();
            context.emitter().emit(diag);

            // Pretend we encountered a closing brace and continue on.
            context.close_state(NodeKind::ClassDefinitionBody, true);
            context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
        }

        // Unexpected end of input.
        None => {
            let last_token = context.last_token();
            let class_def = context
                .close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true)
                .token_index;
            let diag = context
                .emitter()
                .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                    last_token,
                    "Unexpected end of input while parsing class body. Expecting a closing \
                            brace '}'.",
                )
                .note(class_def, "Class defined here.")
                .emit();
            context.emitter().emit(diag);
        }
    };
}
