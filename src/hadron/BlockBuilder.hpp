#ifndef SRC_HADRON_BLOCK_BUILDER_HPP_
#define SRC_HADRON_BLOCK_BUILDER_HPP_

#include "hadron/library/HadronAST.hpp"
#include "hadron/library/HadronCFG.hpp"
#include "hadron/library/HadronHIR.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Symbol.hpp"

#include <memory>

namespace hadron {

class ErrorReporter;
struct ThreadContext;

// Goes from AST to a Control Flow Graph of Blocks of HIR code in SSA form.
//
// This is an implementation of the algorithm described in [SSA2] "Simple and Efficient Construction of Static Single
// Assignment Form" by Bruan M. et al, with modifications to support type deduction while building SSA form.
class BlockBuilder {
public:
    BlockBuilder() = delete;
    BlockBuilder(std::shared_ptr<ErrorReporter> errorReporter);
    ~BlockBuilder() = default;

    library::CFGFrame buildMethod(ThreadContext* context, const library::BlockAST blockAST,
            const library::Class owningClass);

private:
    library::CFGFrame buildFrame(ThreadContext* context, const library::BlockAST blockAST,
            library::BlockLiteralHIR outerBlockHIR);

    // Re-uses the containing stack frame but produces a new scope.
    library::CFGScope buildInlineBlock(ThreadContext* context, library::CFGScope parentScope,
            library::CFGBlock predecessor, const library::BlockAST blockAST);

    library::HIRId buildValue(ThreadContext* context, library::CFGBlock& currentBlock, const library::AST ast);
    library::HIRId buildFinalValue(ThreadContext* context, library::CFGBlock& currentBlock,
            const library::SequenceAST sequence);
    library::HIRId buildIf(ThreadContext* context, library::CFGBlock& currentBlock, const library::IfAST ifAST);
    library::HIRId buildWhile(ThreadContext* context, library::CFGBlock& currentBlock,
            const library::WhileAST whileAST);

    // If |toWrite| is nil that means this is a read operation. Returns nil if name not found.
    library::HIR findName(ThreadContext* context, library::Symbol name, library::CFGBlock block,
            library::HIRId toWrite);

    std::shared_ptr<ErrorReporter> m_errorReporter;
    library::Class m_owningClass;
};

} // namespace hadron

#endif // SRC_HADRON_BLOCK_BUILDER_HPP_