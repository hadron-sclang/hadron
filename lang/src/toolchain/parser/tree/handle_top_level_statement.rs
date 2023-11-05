use super::*;

// root : topLevelStatement* EOF ;
// topLevelStatement : classDef
//                   | classExtension
//                   | interpreterCode
//                   ;
pub fn handle_top_level_statement(context: &mut Context) {
    match context.token_kind() {
        // Class definitions start with a ClassName token.
        Some(TokenKind::ClassName) => {
            context.push_state(NodeKind::ClassDef { kind: ClassDefKind::Root });
        }

        // Class Extensions start with a '+' sign.
        Some(TokenKind::Binop { kind: BinopKind::Plus }) => {
            context.push_state(NodeKind::ClassExtension);
        }

        // Normal expected end of input.
        None => {
            context.pop_state();
        }

        // Everything else we treat as interpreter input.
        _ => context.push_state(NodeKind::InterpreterCode),
    }
}
