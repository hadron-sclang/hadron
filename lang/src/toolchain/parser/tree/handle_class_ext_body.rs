use super::*;

// classExtension  : PLUS CLASSNAME CURLY_OPEN methodDef* CURLY_CLOSE
//                 ;
//
// methodDef : ASTERISK? methodDefName CURLY_OPEN argDecls? varDecls? primitive? body? CURLY_CLOSE ;
//
pub fn handle_class_ext_body(context: &mut Context) {
    debug_assert_eq!(context.state().unwrap().kind, NodeKind::ClassExtensionBody);

    // '{'
    context.consume_checked(TokenKind::Delimiter { kind: DelimiterKind::BraceOpen });

    match context.token_kind() {
        // '}' closes the class extension.
        Some(TokenKind::Delimiter { kind: DelimiterKind::BraceClose }) => {
            context.close_state(NodeKind::ClassExtensionBody, false);
            context.close_state(NodeKind::ClassExtension, false);
            // '}'
            context.consume();
        }

        // Variables and constant declaration only allowed in classes.
        Some(TokenKind::Reserved { kind: ReservedKind::Classvar }) => {
            const_or_var_def_in_class_ext(
                context,
                "Class extensions cannot contain class variable declarations `classvar`. \
                        Did you mean to define a new class instead?",
                NodeKind::ClassVariableDefinition,
            );
        }

        Some(TokenKind::Reserved { kind: ReservedKind::Var }) => {
            const_or_var_def_in_class_ext(
                context,
                "Class extensions cannot contain variable declarations 'var'. Did you \
                    mean to define a new class instead?",
                NodeKind::MemberVariableDefinition,
            );
        }

        Some(TokenKind::Reserved { kind: ReservedKind::Const }) => {
            const_or_var_def_in_class_ext(
                context,
                "Class extensions cannot contain variable declarations 'var'. Did you \
                mean to define a new class instead?",
                NodeKind::MemberVariableDefinition,
            );
        }

        // Namd or binop. Binop could be class method '*' or name of method.
        Some(TokenKind::Identifier { kind: IdentifierKind::Name }) | Some(TokenKind::Binop { kind: _ }) => {
            context.push_state(NodeKind::MethodDefinition);
        }

        // We treat unexpected tokens as a missing brace situation.
        Some(_) => {
            let token_index = context.state().unwrap().token_index;
            let ext_body =
                context.state_parent(1, NodeKind::ClassExtensionBody).unwrap().token_index;
            let ext_def = context.state_parent(2, NodeKind::ClassExtension).unwrap().token_index;
            let diag = context
                .emitter()
                .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                    token_index,
                    "Unexpected token in class extension body. Did you forget a closing \
                        brace '}'?".to_string(),
                )
                .note(ext_body, "Class extension body opened here.".to_string())
                .note(ext_def, "Class extended here.".to_string())
                .emit();
            context.emitter().emit(diag);

            // Pretend we encountered a closing brace and continue on.
            context.close_state(NodeKind::ClassDefinitionBody, true);
            context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, true);
        }

        None => {
            let last_token = context.last_token();
            let ext_def = context.state_parent(2, NodeKind::ClassExtension).unwrap().token_index;
            let diag = context
                .emitter()
                .build(
                    DiagnosticLevel::Error,
                    DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::UnclosedPair },
                    last_token,
                    "Unexpected end of input while parsing class extension body. Expecting a \
                            closing brace '}'.".to_string(),
                )
                .note(ext_def, "Class extended here.".to_string())
                .emit();
            context.emitter().emit(diag);
        }
    }
}

fn const_or_var_def_in_class_ext(context: &mut Context, body: &'static str, kind: NodeKind) {
    let token_index = context.token_index();
    let class_ext = context.state_parent(2, NodeKind::ClassExtension).unwrap().token_index;
    let diag = context
        .emitter()
        .build(
            DiagnosticLevel::Error,
            DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::ConstOrVarDeclInClassExt },
            token_index,
            body.to_string(),
        )
        .note(class_ext, "Class extension declared here".to_string())
        .emit();
    context.emitter().emit(diag);

    context.set_error();
    context.push_state(kind);
    context.push_state(NodeKind::MemberVariableDefinitionList);
}
