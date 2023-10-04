pub struct Node {
    pub kind: NodeKind,
}

pub enum NodeKind {
    ArgList,
    ArrayRead,
    ArrayWrite,
    Assign,
    BinopCall,
    Block,
    Call,
    Class,
    ClassExtension,
    Collection,
    CopySeries,
    Empty,
    EnvironmentAt,
    EnvironmentPut,
    Event,
    ExprSeq,
    If,
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
    While,
    ListComprehension,
    GeneratorQualifier,
    GuardQualifier,
    BindingQualifier,
    SideEffectQualifier,
    TerminationQualifier
}

enum State {
    Foo,
    Bar
}

struct Context {
    pub states: Vec<State>,
}

impl Context {
    pub fn new() -> Context {
        Context { states: Vec<State>::new() }
    }
}

pub fn parse<'a>(lex: Iterator<Item = Token<'a>>) -> impl Iterator<Item = Node> + '_ {

}