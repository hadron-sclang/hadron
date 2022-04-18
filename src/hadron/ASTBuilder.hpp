#ifndef SRC_HADRON_AST_BUILDER_HPP_
#define SRC_HADRON_AST_BUILDER_HPP_

#include "hadron/AST.hpp"
#include "hadron/Slot.hpp"

#include <memory>

namespace hadron {

class ErrorReporter;
class Lexer;
struct ThreadContext;

namespace ast {
struct BlockAST;
}

namespace parse {
struct BlockNode;
struct ExprSeqNode;
struct Node;
}

// The Abstract Syntax Tree (or AST) is a lowering and simplification of the parse tree. Where the parse tree is
// constrained to strictly represent the input source code as parsed, the AST drops that requirement and so can simplify
// much of the "syntatic sugar" in the SuperCollider language. It also lends itself to tree manipulation, which some
// forms of optimization are easier to express in. The AST is also the first stage of the compiler that introduces
// garbage-collected objects, allowing the source code to be unloaded after building. There are no null pointers in a
// valid AST.

class ASTBuilder {
public:
    ASTBuilder();
    ASTBuilder(std::shared_ptr<ErrorReporter> errorReporter);
    ~ASTBuilder() = default;

    // We only build AST from Blocks, leaving the higher-level language constructs (like Classes) behind.
    std::unique_ptr<ast::BlockAST> buildBlock(ThreadContext* context, const Lexer* lexer,
            const parse::BlockNode* blockNode);

    // Node can be a LiteralNode, SymbolNode, or one or more StringNodes. Sets errors in errorReporter if parsing error.
    Slot buildLiteral(ThreadContext* context, const Lexer* lexer, const parse::Node* node);

private:
    void appendToSequence(ThreadContext* context, const Lexer* lexer, ast::SequenceAST* sequence,
            const parse::Node* node);
    std::unique_ptr<ast::AST> transform(ThreadContext* context, const Lexer* lexer, const parse::Node* node);
    std::unique_ptr<ast::SequenceAST> transformSequence(ThreadContext* context, const Lexer* lexer,
            const parse::ExprSeqNode* exprSeqNode);

    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_HADRON_AST_BUILDER_HPP_