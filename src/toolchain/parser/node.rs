use crate::toolchain::lexer::tokenized_buffer::TokenIndex;

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

#[derive(Debug, Eq, PartialEq)]
pub enum NodeKind {
    ArgList,
    ArrayRead,
    ArrayWrite,
    Assign,
    BinopCall,
    Block,
    Call,

    /// A class defintion:
    ///     _optional_: Name: identifier token
    ///   _optional_: ClassArrayStorageType: '[' ']'
    ///     Name: classname token
    ///   _optional_: ClassSuperclass: ':'
    ///   _external_: ClassDefinitionBody: '{' '}'
    /// ClassDefinition: classname '}'
    ClassDefinition,
    ClassArrayStorageType,
    ClassSuperclass,

    /// The body of a class definition:
    ///   _optional_, _external_: MethodDef: name '}'
    ///   _optional_, _external_: ConstDef: 'const' ';'
    ///   _optional_, _external_: VarDef: 'var' ';'
    ///   _optional_, _external_: ClassVarDef: 'classvar' ';'
    /// ClassDefinitionBody: '{' '}'
    ///
    /// All the children of ClassDefinitionBody may occur zero or more times, in any order.
    ClassDefinitionBody,

    ClassExtension,

    Collection,
    CopySeries,
    Empty,
    EnvironmentAt,
    EnvironmentPut,
    Event,
    ExprSeq,
    KeyValue,
    Method,
    MultiAssign,
    MultiAssignVars,

    Name,

    New,
    NumericSeries,
    PerformList,
    PutSeries,
    Return,
    Series,
    SeriesIter,
    Setter,
    Slot,
    String,
    Symbol,
    Value,
    VarDef,
    VarList,
    ListComprehension,
    GeneratorQualifier,
    GuardQualifier,
    BindingQualifier,
    SideEffectQualifier,
    TerminationQualifier,
}
