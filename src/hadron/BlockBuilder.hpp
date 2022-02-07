#ifndef SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/library/Symbol.hpp"

#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace hadron {

struct Block;
class ErrorReporter;
struct Frame;
struct Scope;
struct ThreadContext;

namespace ast {
struct AST;
struct BlockAST;
struct IfAST;
struct SequenceAST;
} // namespace parse

// Goes from AST to a Control Flow Graph of Blocks of HIR code in SSA form.
//
// This is an implementation of the algorithm described in [SSA2] "Simple and Efficient Construction of Static Single
// Assignment Form" by Bruan M. et al, with modifications to support type deduction while building SSA form.
class BlockBuilder {
public:
    BlockBuilder() = delete;
    BlockBuilder(std::shared_ptr<ErrorReporter> errorReporter);
    ~BlockBuilder();

    std::unique_ptr<Frame> buildFrame(ThreadContext* context, const ast::BlockAST* blockAST);

private:
    // Re-uses the containing stack frame but produces a new scope. Needs exactly one predecessor.
    std::unique_ptr<Scope> buildInlineBlock(ThreadContext* context, Block* predecessor, const ast::BlockAST* blockAST);

    // Take the expression sequence in |node|, build SSA form out of it, return pair of value numbers associated with
    // expression value and expression type respectively. While it will process all descendents of |node| it will not
    // iterate to process the |node->next| pointer. Call buildFinalValue() to do that.
    hir::NVID buildValue(ThreadContext* context, Block*& currentBlock, const ast::AST* ast);
    hir::NVID buildFinalValue(ThreadContext* context, Block*& currentBlock,
            const ast::SequenceAST* sequence);
    hir::NVID buildIf(ThreadContext* context, Block*& currentBlock, const ast::IfAST* ifAST);

    // Returns the value either inserted or re-used (if a constant). Takes ownership of hir.
    hir::NVID insert(std::unique_ptr<hir::HIR> hir, Block* block);

    // Recursively traverse through blocks looking for recent revisions of the value and type. Then do the phi insertion
    // to propagate the values back to the currrent block. Also needs to insert the name into the local block revision
    // tables.
    hir::NVID findName(library::Symbol name, Block* block,
                std::unordered_map<int32_t, hir::NVID>& blockValues,
                const std::unordered_set<const Scope*>& containingScopes);

    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_