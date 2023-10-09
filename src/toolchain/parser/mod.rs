//! So much stuff to do:
//! [ ] Figure out namespaces and crate structures
//! [ ] TokenizedBuffer? Should Token become an index type, rename existing to TokenInfo?
//! [ ] How to get `line` &str into the token?
//! [ ] Diagnostics usability/macros/emission
//! [ ] Implement parsing state machine
//! [ ] Parsing state machine in other files (expr!)
//! [ ] Parser unit/smoke tests
//! [ ] Documentation
//! [ ] Integration testing?
//! [ ] After merge - fuzz testing
//! [ ] After merge - benchmarks for lexer and parser + "speed of light"

use crate::toolchain::diagnostics;
use crate::toolchain::lexer;

pub fn parse<'a>(&mut lex: Iterator<Item = lexer::Token<'a>>, &mut diags: diagnostics::DiagnosticConsumer) -> Tree<'a> {
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

                // Check for array type indicator, if present.


                match lex_iter.front().kind {
                }
            }
        }
    }

    tree
}


#[cfg(test)]
mod tests {
    use super::parse;

    fn check_parse(src: &str, expect: &str) {
        let mut lexer = lexer::tokenize(str);
        let tree = parse(lexer);
    }
}