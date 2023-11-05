use crate::toolchain::lexer::TokenIndex;

#[derive(Debug, Eq, PartialEq)]
pub struct Node {
    pub kind: NodeKind,
    pub token_index: TokenIndex,
    pub subtree_size: TokenIndex,
    // For matched pairs of delimiters, like '()' and '{}', indicates the index of the closing
    // token, if present.
    pub closing_token: Option<TokenIndex>,
    pub has_error: bool,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum NodeKind {
    /// A class defintion:
    ///     _optional_: Name: identifier token
    ///   _optional_: ClassDef { ArrayStorageType }: '[' ']'
    ///     Name: classname token
    ///   _optional_: ClassDef { Superclass }: ':'
    ///   _external_: ClassDefinitionBody: '{' '}'
    /// ClassDef { Root }: classname '}'
    ClassDef {
        kind: ClassDefKind,
    },

    /// The body of a class definition:
    ///   _optional_, _external_: MethodDef: name '}'
    ///   _optional_, _external_: ConstDef: 'const' ';'
    ///   _optional_, _external_: VarDef: 'var' ';'
    ///   _optional_, _external_: ClassVarDef: 'classvar' ';'
    /// ClassDefinitionBody: '{' '}'
    ///
    /// All the children of ClassDefinitionBody may occur zero or more times, in any order.
    ClassDefinitionBody,

    /// A class extension:
    ///   Name: classname
    ///   _external_: ClassExtensionBody: '{' '}'
    /// ClassExtension: '+'
    ClassExtension,

    
    ClassExtensionBody,

    /// A 'classvar' class variable definition:
    ///   _external_: MemberVariableDefinitionList
    /// ClassVariableDefinition: 'classvar' ';'
    ClassVariableDefinition,

    ConstantDefinition,

    InterpreterCode,

    MethodDefinition,

    /// A 'var' variable definition inside a class:
    ///   _external_: MemberVariableDefinitionList
    /// MemberVariableDefinition: 'var' ';'
    MemberVariableDefinition,

    MemberVariableDefinitionList,

    Name,

    /// The top-level statement tree in any sclang code:
    ///   _optional_, _external_: ClassDefinition: Classname '}'
    ///   _optional_, _external_: ClassExtension: '+' '}'
    ///   _optional_, _external_: InterpreterCode
    /// TopLevelStatement
    TopLevelStatement,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ClassDefKind {
    Root,
    Superclass,
    ArrayStorageType,
}
