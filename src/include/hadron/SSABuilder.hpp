#ifndef SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_
#define SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Slot.hpp"

#include <cassert>
#include <list>
#include <memory>
#include <unordered_map>
#include <vector>

namespace hadron {

class ErrorReporter;
class Lexer;

namespace parse {
struct BlockNode;
struct Node;
} // namespace parse

struct Frame;
struct Block {
    Block(Frame* owningFrame, int blockNumber): frame(owningFrame), number(blockNumber) {}

    // value numbers are frame-wide but for LVN the value lookups are block-local, because extra-block values
    // need to go through a psi function in this Block.
    std::unordered_map<int32_t, hir::HIR*> values;

    Frame* frame;
    int number;
    std::list<Block*> predecessors;
    std::list<std::unique_ptr<hir::HIR>> statements;
};

// Represents a stack frame, so can have arguments supplied, is a scope for local variables, has an entrance and exit
// Block.
struct Frame {
    Frame(): parent(nullptr) {}
    ~Frame() = default;

    // In-order hashes of argument names.
    std::vector<Hash> argumentOrder;
    // References to the HIR opcodes for each revision of each named value (variables and arguments)
    std::unordered_map<Hash, std::vector<hir::HIR*>> revisions;

    Frame* parent;
    std::list<std::unique_ptr<Block>> blocks;

    // Modifies revision number of op
    void addRevision(hir::HIR* op) {
        auto iter = revisions.find(op->target);
        assert(iter != revisions.end());
        op->revision = static_cast<int32_t>(iter->second.size());
        iter->second.emplace_back(op);
    }
};

// Goes from parse tree to HIR in blocks of HIR in SSA form.
class SSABuilder {
public:
    SSABuilder(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter);
    ~SSABuilder();

    std::unique_ptr<Frame> build(const Lexer* lexer, const parse::BlockNode* blockNode);

private:
    // Recursively traverse the parse tree rooted at |node|, placing HIR into the current m_block, and updating m_frame
    // and m_block as necessary.
    void fillBlock(const parse::Node* node);
    // Decompose the expression sequence at value to a series of expressions, then assign the final value to the next
    // version of name.
    void buildAssign(Hash name, const parse::Node* value);
    // Assign a constnat value to the latest version of the value.
    void buildAssignConstant(Hash name, const Slot& slot);

    Lexer* m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    Frame* m_frame;
    Block* m_block;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_