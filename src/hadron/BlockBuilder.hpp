#ifndef SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_

#include "hadron/Block.hpp"
#include "hadron/hir/HIR.hpp"
#include "hadron/library/Kernel.hpp"
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
struct WhileAST;
}

namespace hir {
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
    ~BlockBuilder();

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

    void sealBlock(ThreadContext* context, const library::Method method, Block* block);

    // Returns the value appended to the |block|. Takes ownership of hir.
    hir::ID append(std::unique_ptr<hir::HIR> hir, Block* block);
    hir::ID insert(std::unique_ptr<hir::HIR> hir, Block* block,
            std::list<std::unique_ptr<hir::HIR>>::iterator before);

    // Follow order of precedence in names to locate an identifer symbol, including in local variables, arguments,
    // instance variables, class variables, and pre-defined identifiers. Can return hir::kInvalidID, which means a
    // compilation error that the name is not found.
    hir::ID findName(ThreadContext* context, const library::Method method, library::Symbol name, Block* block);

    // Recursively traverse through blocks looking for recent revisions of the value and type. Then do the phi insertion
    // to propagate the values back to the currrent block. Also needs to insert the name into the local block revision
    // tables. Can return hir::kInvalidID which means the name was not found.
    hir::ID findScopedName(ThreadContext* context, library::Symbol name, Block* block);
    hir::ID findScopedNameRecursive(ThreadContext* context, library::Symbol name, Block* block,
                                      std::unordered_map<Block::ID, hir::ID>& blockValues,
                                      const std::unordered_set<const Scope*>& containingScopes,
                                      std::unordered_map<hir::HIR*, hir::HIR*>& trivialPhis);

    // Replaces pairs (key, value). May cause other replacements, which are handled sequentially. Destructively modifies
    // the |replacements| map.
    void replaceValues(std::unordered_map<hir::HIR*, hir::HIR*>& replacements);

    std::shared_ptr<ErrorReporter> m_errorReporter;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_