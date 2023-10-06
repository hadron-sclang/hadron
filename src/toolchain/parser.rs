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
    TopLevelStatement,


    ClassDefStart,

    ClassExtDef,
    InterpreterCode,
//    ClassVarDecl,
}

struct Context<'a> {
    pub state: State,
    pub tree_start: i32,
}

// A parse tree.
pub struct Tree<'a> {
    nodes: Vec<Node>,
}

impl<'a> Tree<'a> {
    pub fn new() -> Tree<'a> {
        Tree { nodes: Vec<Node>::new() }
    }

    pub fn size(&self) -> usize {
        self.nodes.size()
    }


}

pub fn parse<'a>(&mut lex: Iterator<Item = lexer::Token<'a>>, &mut diags: DiagnosticConsumer) -> Tree<'a> {
    let mut tree = Tree::new();
    let mut lex_iter = LexIter::new(lex);
    let mut stack = Vec<Context>::new();
    stack.push(Context { state: State::RootLevel, tree_start: 0 });

    while !stack.empty() {
        let &context = stack.last().expect("non-empty stack should have valid last element.");
        match context.state {
            // topLevelStatement : classDef
            //                   | classExtension
            //                   | interpreterCode
            //                   ;
            State::TopLevelStatement => {
                match lex_iter.front().kind {
                    lexer::TokenKind::ClassName => {
                        stack.push(Context { state: State::ClassDefStart, tree.size() });
                    },
                    lexer::TokenKind::Binop { kind: BinopKind::Plus } => {
                        stack.push(Context { state: State::ClassExtDef, tree.size() });
                    },
                    lexer::TokenKind::EndOfInput => {
                        // Normal end of a parse tree, terminate.
                        stack.pop();
                    },
                    _ => {
                        stack.push(Context { state: State::InterpreterCode, tree.size() });
                    }
                }
            },

            // classDef : CLASSNAME superclass? CURLY_OPEN classVarDecl* methodDef* CURLY_CLOSE
            //          | CLASSNAME SQUARE_OPEN name? SQUARE_CLOSE superclass? CURLY_OPEN classVarDecl* methodDef* CURLY_CLOSE
            State::ClassDefStart => {
                debug_assert!(lex_iter.front().kind == lexer::TokenKind::ClassName);

                match lex_iter.front().kind {
                }
            }
        }
    }

    tree
}

// A peekable iterator over the lexer tokens that skips empty space.
struct LexIter<'a> {
    lexer: Iterator<Item = lexer::Token<'a>>,
    token: lexer::Token<'a>,
}

impl<'a> LexIter<'a> {
    pub fn new(lexer: Iterator<Item = lexer::Token<'a>>) -> LexIter<'a> {
        // Extract first token for peeking, if it exists.
        let token = match lexer.next() {
            Some(t) => t,
            None => lexer::Token::end(),
        };
        let lex_iter = LexIter { lexer, token };
        lex_iter.advance_to_nonempty();
        lex_iter
    }

    pub fn first(&self) -> &Token<'a> {
        self.token
    }

    pub fn bump(&mut self) -> Option<Token<'a>> {
        if token.kind == lexer::TokenKind::EndOfInput {
            None
        } else {
            let prev = self.token;
            advance_to_nonempty();
            prev
        }
    }

    fn advance_to_nonempty(&mut self) {
        while is_empty(self.token.token_kind) {
            self.token = match lexer.next() {
                Some(t) => t,
                None => lexer::Token::end()
            };
        }
    }

    fn is_empty(kind: lexer::TokenKind) -> bool {
        matches!(kind,
            lexer::TokenKind::BlankSpace,
            lexer::TokenKind::BlockComment,
            lexer::TokenKind::LineComment)
    }
}


#[cfg(test)]
mod tests {
    use super::parse;

    fn check_parse(src: &str, expect: &str) {
        let mut lexer = lexer::tokenize(str);
        let tree = parse(lexer);
    }
}