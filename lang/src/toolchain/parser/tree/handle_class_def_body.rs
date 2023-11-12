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
    context.consume_checked(TokenKind::Group { kind: GroupKind::BraceOpen });

    expect!(
        context,
        TokenKind::Group { kind: GroupKind::BraceClose },
        {
            // Closing brace, end of class def body.
            context.close_state(NodeKind::ClassDefinitionBody, false);
            context.close_state(NodeKind::ClassDef { kind: ClassDefKind::Root }, false);
            // '}'
            context.consume();
        },
        TokenKind::Reserved { kind: ReservedKind::Classvar },
        {
            // 'classvar'
            context.push_state(NodeKind::ClassVariableDefinition);
            context.consume();

            context.push_state(NodeKind::ReadWriteVariableDef);
        },
        TokenKind::Reserved { kind: ReservedKind::Var },
        {
            // 'var'
            context.push_state(NodeKind::MemberVariableDefinition);
            context.consume();

            context.push_state(NodeKind::ReadWriteVariableDef);
        },
        TokenKind::Reserved { kind: ReservedKind::Const },
        {
            // 'const'
            context.push_state(NodeKind::ConstantDefinition);
        },
        TokenKind::Identifier { kind: IdentifierKind::Name } | TokenKind::Binop { kind: _ },
        {
            // name of method or '*' indicatating class method.
            context.push_state(NodeKind::MethodDefinition);
        }
    );
}
