use super::*;

pub fn handle_literal(context: &mut Context) {
    debug_assert!(context.state().unwrap().kind == NodeKind::Literal);

    if !context.consume_literal() {
        context.recover_unexpected_token();
    }
}
