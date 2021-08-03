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

    // Value numbers are frame-wide but for LVN the value lookups are block-local, because extra-block values
    // need to go through a psi function in this Block.
    std::unordered_map<int32_t, hir::HIR*> values;
    // Map of named values to value numbers
    std::unordered_map<Hash, int32_t> nameRevisions;
    std::unordered_map<Hash, int32_t> typeRevisions;

    Frame* frame;
    int number;
    std::list<Block*> predecessors;
    std::list<std::unique_ptr<hir::HIR>> statements;
};

// Represents a stack frame, so can have arguments supplied, is a scope for local variables, has an entrance and exit
// Block.
struct Frame {
    Frame(): valueCount(0), parent(nullptr) {}
    ~Frame() = default;

    // In-order hashes of argument names.
    std::vector<Hash> argumentOrder;

    // Local Value Numbering uses this to continue to increment value numbering.
    int32_t valueCount;

    Frame* parent;
    std::list<std::unique_ptr<Block>> blocks;
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

    // Take the expression sequence in |node|, build SSA form out of it, return pair of value numbers associated with
    // expression value and expression type respectively.
    std::pair<int32_t, int32_t> buildSSA(const parse::Node* node);

    // Algorithm is to iterate through all previously defined values *in the block* to see if they have already defined
    // an identical value.
    int32_t findOrInsert(std::unique_ptr<hir::HIR> hir);

    Lexer* m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    Frame* m_frame;
    Block* m_block;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_