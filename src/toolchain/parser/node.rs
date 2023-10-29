use crate::toolchain::lexer::tokenized_buffer::TokenIndex;

pub struct Node {
    pub kind: NodeKind,
    pub token_index: TokenIndex,
    pub subtree_size: TokenIndex,
    // For matched pairs of delimiters, like '()' and '{}', indicates the index of the closing
    // token, if present.
    pub closing_token: Option<TokenIndex>,
    pub has_error: bool
}

pub enum NodeKind {
    ArgList,
    ArrayRead,
    ArrayWrite,
    Assign,
    BinopCall,
    Block,
    Call,
    ClassArrayStorageType,
    ClassDefinition,
    ClassDefinitionBody,
    ClassExtension,
    ClassName,
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
    Superclass,
    Symbol,
    Value,
    VarDef,
    VarList,
    ListComprehension,
    GeneratorQualifier,
    GuardQualifier,
    BindingQualifier,
    SideEffectQualifier,
    TerminationQualifier
}
