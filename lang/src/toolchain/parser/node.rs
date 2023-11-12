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
    ///
    /// ```text
    ///     _optional_: Name: identifier token
    ///   _optional_: ClassDef { ArrayStorageType }: '[' ']'
    ///     Name: classname
    ///   _optional_: ClassDef { Superclass }: ':'
    ///   _external_: ClassDefinitionBody: '{' '}'
    /// ClassDef { Root }: classname '}'
    /// ```
    ClassDef {
        kind: ClassDefKind,
    },

    /// The body of a class definition:
    ///
    /// ```text
    ///   _optional_, _external_: MethodDef: name '}'
    ///   _optional_, _external_: ConstDef: 'const' ';'
    ///   _optional_, _external_: VarDef: 'var' ';'
    ///   _optional_, _external_: ClassVarDef: 'classvar' ';'
    /// ClassDefinitionBody: '{' '}'
    /// ```
    ///
    /// All the children of ClassDefinitionBody may occur zero or more times, in any order.
    ClassDefinitionBody,

    /// A class extension:
    ///
    /// ```text
    ///   Name: classname
    ///   _external_: ClassExtensionBody: '{' '}'
    /// ClassExtension: '+' '}'
    /// ```
    ClassExtension,

    /// The body of a class extension:
    ///
    /// ```text
    ///   _optional_, _external_: MethodDef: name '}'
    /// ClassExtensionBody: '{' '}'
    /// ```
    ///
    /// MethodDef children may occur zero or more times.
    ClassExtensionBody,

    /// A 'classvar' class variable definition:
    ///
    /// ```text
    ///   _external_: ReadWriteVariableDef
    /// ClassVariableDefinition: 'classvar' ';'
    /// ```
    ///
    /// The ReadWriteVariableDef must appear at least once, but can repeat.
    ClassVariableDefinition,

    ConstantDefinition,

    /// The initial value of a class, member, or local variable:
    ///
    /// ```text
    ///   Literal: 1 or more tokens
    /// InitialValueSpecifier: '='
    /// ```
    InitialValueSpecifier,

    InterpreterCode,

    /// A literal value in the source. A leaf node consisting of 1 or more tokens.
    Literal,

    MethodDefinition,

    /// A 'var' variable definition inside a class:
    ///
    /// ```text
    ///   _external_: ReadWriteVariableDef
    /// MemberVariableDefinition: 'var' ';'
    /// ```
    ///
    /// The ReadWriteVariableDef must appear at least once, but can repeat.
    MemberVariableDefinition,

    /// The complete specification of a class or member variable:
    ///
    /// ```text
    ///   _optional_: AccessSpecification: '<', '>', or '<>'
    ///   Name: identifier
    ///   _optional_, _external_: InitialValueSpecifier
    /// ReadWriteVariableDef
    /// ```
    ReadWriteVariableDef,

    /// A ClassName or an identifier, depending on context. Always a leaf node.
    Name,

    /// The top-level statement tree in any sclang code:
    ///
    /// ```text
    ///   _optional_, _external_: ClassDefinition: Classname '}'
    ///   _optional_, _external_: ClassExtension: '+' '}'
    ///   _optional_, _external_: InterpreterCode
    /// TopLevelStatement
    /// ```
    TopLevelStatement,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ClassDefKind {
    Root,
    Superclass,
    ArrayStorageType,
}
