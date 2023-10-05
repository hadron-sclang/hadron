use crate::toolchain::lexer;

pub struct Node<'a> {
    pub kind: NodeKind,
    pub token: lexer::Token<'a>,
    pub subtree_size: u32,
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

// A parse tree.
pub struct Tree<'a> {
    nodes: Vec<Node>,
}

impl<'a> Tree<'a> {
    pub fn new() -> Tree<'a> {
        Tree { nodes: Vec<Node>::new() }
    }
}

pub fn parse<'a>(lex: Iterator<Item = lexer::Token<'a>>) -> Tree<'a> {
    let mut tree = Tree::new();
    tree
}

// A peekable iterator over the lexer tokens that skips empty space and unknown 
struct TokenIterator {

}


#[cfg(test)]
mod tests {
    use super::parse;

    fn check_parse(src: &str, expect: &str) {
        let mut lexer = lexer::tokenize(str);
        let tree = parse(lexer);
    }
}