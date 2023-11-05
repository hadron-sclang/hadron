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
        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Classvar }) => {
            const_or_var_def_in_class_ext(
                context,
                   "Class extensions cannot contain class variable declarations `classvar`. \
                        Did you mean to define a new class instead?",
          NodeKind::ClassVariableDefinition);
        }

        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Var }) => {
            const_or_var_def_in_class_ext(
                context,
                "Class extensions cannot contain variable declarations 'var'. Did you \
                    mean to define a new class instead?",
                NodeKind::MemberVariableDefinition);
        }

        Some(TokenKind::ReservedWord { kind: ReservedWordKind::Const }) => {
            const_or_var_def_in_class_ext(
                context,
                "Class extensions cannot contain variable declarations 'var'. Did you \
                mean to define a new class instead?",
                NodeKind::MemberVariableDefinition);
        }

        // Identifier or binop. Binop could be class method '*' or name of method.
        Some(TokenKind::Identifier) | Some(TokenKind::Binop { kind: _ }) => {
            context.push_state(NodeKind::MethodDefinition);
        }

        Some(_) => {

        }

        None => {
            
        }
    }
}

fn const_or_var_def_in_class_ext(context: &mut Context, body: &'static str, kind: NodeKind) {
    let token_index = context.token_index();
    let class_ext = context
        .state_parent(2, NodeKind::ClassExtension)
        .unwrap()
        .token_index;
    let diag = context
        .emitter()
        .build(
            DiagnosticLevel::Error,
            DiagnosticKind::SyntaxError { kind: SyntaxDiagnosticKind::ConstOrVarDeclInClassExt },
            token_index,
            body,
        )
        .note(class_ext, "Class extension declared here")
        .emit();
    context.emitter().emit(diag);

    context.set_error();
    context.push_state(kind);
    context.push_state(NodeKind::MemberVariableDefinitionList);
}