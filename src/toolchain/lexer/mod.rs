//! A SuperCollider language lexer, which splits the input sclang string into tokens for parsing.
//!
//! This lexer provides light-weight [Token] structures via the iterator provided by the
//! [tokenize()] method. In addition to the parser front end, this lexer intends to support other
//! use cases like tokenization for an LSP server or other user-in-the-loop runtime analysis
//! services. To do that, it must be fast and tolerant of erroneous or incomplete input. The lexer
//! does no validation or processing of the input string other than tokenization; for example, it
//! does not convert number strings into their binary representation. The compiler defers further
//! work until the parsing or semantic translation steps.
//!
//! The rustc and Carbon lexers inspired this design.
//!

pub mod token;
pub mod tokenized_buffer;

mod cursor;
