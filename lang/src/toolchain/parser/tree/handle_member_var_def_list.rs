use super::*;

// rwSlotDefList : rwSlotDef (COMMA rwSlotDef)* ;
//
// rwSlotDef : rwSpec? name (EQUALS literal)? ;
//
// rwSpec : LESS_THAN
//        | GREATER_THAN
//        | READ_WRITE
//        ;
pub fn handle_member_var_def_list(context: &mut Context) {
    debug_assert_eq!(context.state().unwrap().kind, NodeKind::MemberVariableDefinitionList);

    

}
