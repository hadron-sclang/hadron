use super::*;

// classVarDecl : CLASSVAR rwSlotDefList SEMICOLON
//              | VAR rwSlotDefList SEMICOLON
//              | CONST constDefList SEMICOLON
;

pub fn handle_class_var_def(context: &mut Context) {
    debug_assert_eq!(context.state().unwrap().kind, NodeKind::ClassVariableDefinition);

    // semicolon..
}
