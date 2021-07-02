# Hadron Compilation Stages

## Lexing produces a Token stream

The source is lexed into tokens using a token analyzer state machine built using Ragel. Using Lexer.rl as input, Ragel
generates Lexer.cpp at build time, which contains the lexing state machine.

## Parsing produces a Parse Tree

The goal of parsing in Hadron is to produce a *parse tree* only, meaning a tree representation of the input tokens
identified by the lexer. This is different from LSC which lexes, parses, and does some modifications to the parse tree
all in one pass. The parse tree is built by the Parser defined in Parser.cpp.

## Syntax Analysis produces an Abstract Syntax Tree (AST)

The Syntax Analyzer uses the parse tree to build a lower-level Abstract Syntax Tree (AST), in which nodes have no
required relationship to the input source. This AST is used to identify control-flow structures (`if`, `while`, `do`),
locate scoped variables, and perform type inference on values. The Syntax Analyzer object is found in
SyntaxAnalyzer.cpp.

It would be cool if we could drop the original source code after this, so any string_view references to the code can't
live past the Parse Tree.

## AST Optimizations and Tree Transformations

Type deduction and code lowering can happen here - also dead code elimination, others.


## Code Generation produces a Control Flow Graph of Blocks with HIR

Since the AST has variable references organized with type deductions in place, as well as control flow structures
readily identified, it is now possible to generate Blocks of code containing the lower-level Hadron Intermediate
Representation (HIR) defined. These blocks are organized into a Control Flow Graph form.

## LLVM IR Backend Code Generation produces LLVM IR for (possibly AoT only) Compilation

CFG => SSA => LLVM IR => LLVM => Machine Code

## GNU Lightning Backend Code (fast(er) path)

CFG => Register Spilling => GNU Lightning IR => Machine Code
