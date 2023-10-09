enum State {
    TopLevelStatement,


    ClassDefStart,

    ClassExtDef,
    InterpreterCode,
//    ClassVarDecl,
}

struct StateStackEntry {
   pub state: State,
   pub token_index: i32,
   pub subtree_start: i32,
}

pub struct Context<'a> {
    states: Vec<StateStackEntry>,
}


// A peekable iterator over the lexer tokens that skips empty space.
// TODO: merge into Context I should say

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