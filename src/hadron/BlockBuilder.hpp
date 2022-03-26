#ifndef SRC_HADRON_BLOCK_BUILDER_HPP_
#define SRC_HADRON_BLOCK_BUILDER_HPP_

#include "hadron/Block.hpp"
#include "hadron/hir/HIR.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Symbol.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace hadron {

class Block;
class ErrorReporter;
struct Frame;
struct Scope;
struct ThreadContext;

namespace ast {
struct AST;
struct BlockAST;
struct IfAST;
struct SequenceAST;
struct WhileAST;
}

namespace hir {
struct AssignHIR;
struct BlockLiteralHIR;
}

// Goes from AST to a Control Flow Graph of Blocks of HIR code in SSA form.
//
// This is an implementation of the algorithm described in [SSA2] "Simple and Efficient Construction of Static Single
// Assignment Form" by Bruan M. et al, with modifications to support type deduction while building SSA form.
class BlockBuilder {
public:
    BlockBuilder() = delete;
    BlockBuilder(std::shared_ptr<ErrorReporter> errorReporter);
    ~BlockBuilder() = default;

    std::unique_ptr<Frame> buildMethod(ThreadContext* context, const library::Method method,
            const ast::BlockAST* blockAST);

private:
    std::unique_ptr<Frame> buildFrame(ThreadContext* context, const library::Method method,
            const ast::BlockAST* blockAST, hir::BlockLiteralHIR* outerBlockHIR);

    // Re-uses the containing stack frame but produces a new scope.
    std::unique_ptr<Scope> buildInlineBlock(ThreadContext* context, const library::Method method, Scope* parentScope,
            Block* predecessor, const ast::BlockAST* blockAST, bool isSealed = true);

    // Take the expression sequence in |node|, build SSA form out of it, return pair of value numbers associated with
    // expression value and expression type respectively. While it will process all descendents of |node| it will not
    // iterate to process the |node->next| pointer. Call buildFinalValue() to do that.
    hir::ID buildValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
            const ast::AST* ast);
    hir::ID buildFinalValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
            const ast::SequenceAST* sequence);
    hir::ID buildIf(ThreadContext* context, const library::Method method, Block*& currentBlock,
            const ast::IfAST* ifAST);
    hir::ID buildWhile(ThreadContext* context, const library::Method method, Block*& currentBlock,
            const ast::WhileAST* whileAST);

    // if |toWrite| is kInvalidID that means this is a read operation.
    hir::ID findName(ThreadContext* context, library::Symbol name, Block* block, hir::ID toWrite);

    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_HADRON_BLOCK_BUILDER_HPP_