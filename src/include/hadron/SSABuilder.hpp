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
struct KeyValueNode;
struct Node;
} // namespace parse

struct Frame;
struct Block {
    Block(Frame* owningFrame, int blockNumber): frame(owningFrame), number(blockNumber) {}

    // Value numbers are frame-wide but for LVN the value lookups are block-local, because extra-block values
    // need to go through a psi function in this Block.
    std::unordered_map<int32_t, hir::HIR*> values;
    // Map of named values to value numbers TODO: switch to single map of pairs
    std::unordered_map<Hash, int32_t> nameRevisions;
    std::unordered_map<Hash, int32_t> typeRevisions;

    Frame* frame;
    int number;
    std::list<Block*> predecessors;
    std::list<Block*> successors;
    std::list<std::unique_ptr<hir::HIR>> statements;
};

// Represents a stack frame, so can have arguments supplied, is a scope for local variables, has an entrance and exit
// Block.
struct Frame {
    Frame(): parent(nullptr) {}
    ~Frame() = default;

    // In-order hashes of argument names.
    std::vector<Hash> argumentOrder;

    Frame* parent;
    std::list<std::unique_ptr<Block>> blocks;
    std::list<std::unique_ptr<Frame>> subFrames;
};

// Goes from parse tree to HIR in blocks of HIR in SSA form.
class SSABuilder {
public:
    SSABuilder(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter);
    ~SSABuilder();

    std::unique_ptr<Frame> buildFrame(const parse::BlockNode* blockNode);

private:
    // Take the expression sequence in |node|, build SSA form out of it, return pair of value numbers associated with
    // expression value and expression type respectively. While it will process all descendents of |node| it will not
    // iterate to process the |node->next| pointer. Call buildFinalValue() to do that.
    std::pair<int32_t, int32_t> buildValue(const parse::Node* node);
    std::pair<int32_t, int32_t> buildFinalValue(const parse::Node* node);
    std::pair<int32_t, int32_t> buildDispatch(const parse::Node* target, Hash selector, const parse::Node* arguments,
        const parse::KeyValueNode* keywordArguments);

    // Algorithm is to iterate through all previously defined values *in the block* to see if they have already defined
    // an identical value. Returns the value either inserted or re-used. Takes ownership of hir.
    int32_t findOrInsert(std::unique_ptr<hir::HIR> hir);
    int32_t insert(std::unique_ptr<hir::HIR> hir);

    // Recursively traverse through blocks looking for recent revisions of the value and type. Then do the phi insertion
    // to propagate the values back to the currrent block. Also needs to insert the name into the local block revision
    // tables.
    std::pair<int32_t, int32_t> findName(Hash name);


    Lexer* m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    Frame* m_frame;
    Block* m_block;
    int m_blockSerial; // huh, what happens if blocks and values get same serial numbers?
    int32_t m_valueSerial;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_