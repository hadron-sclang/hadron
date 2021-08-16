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

// The Value pair tracks a value number and a flag set that is possible types. The problem is for any situation where
// the type is unknown we also need a value to track the runtime type. So things like buildValue need to return a
// pair of <Value, Value> to track value and *type value*. Which is exhausting but there you have it.

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
    // need to go through a Phi function in this Block. For local value numbering we keep a map of the
    // value to the associated HIR instruction, for possible re-use of instructions.
    std::unordered_map<Value, hir::HIR*> values;
    // Map of names (variables, arguments) to most recent revision of <values, type>
    std::unordered_map<Hash, std::pair<Value, Value>> revisions;

    // Map of values defined extra-locally and their local value. For convenience we also put local values in here,
    // mapping to themselves.
    std::unordered_map<Value, Value> localValues;


    // Owning frame of this block.
    Frame* frame;
    // Unique block number.
    int number;
    std::list<Block*> predecessors;
    std::list<Block*> successors;

    std::list<std::unique_ptr<hir::PhiHIR>> phis;
    // Statements in order of execution.
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
    std::unique_ptr<Frame> buildSubframe(const parse::BlockNode* blockNode);

    // Take the expression sequence in |node|, build SSA form out of it, return pair of value numbers associated with
    // expression value and expression type respectively. While it will process all descendents of |node| it will not
    // iterate to process the |node->next| pointer. Call buildFinalValue() to do that.
    std::pair<Value, Value> buildValue(const parse::Node* node);
    std::pair<Value, Value> buildFinalValue(const parse::Node* node);
    std::pair<Value, Value> buildDispatch(const parse::Node* target, Hash selector, const parse::Node* arguments,
            const parse::KeyValueNode* keywordArguments);

    // Algorithm is to iterate through all previously defined values *in the block* to see if they have already defined
    // an identical value. Returns the value either inserted or re-used. Takes ownership of hir.
    Value findOrInsertLocal(std::unique_ptr<hir::HIR> hir);
    Value insertLocal(std::unique_ptr<hir::HIR> hir);
    Value insert(std::unique_ptr<hir::HIR> hir, Block* block);

    // Recursively traverse through blocks looking for recent revisions of the value and type. Then do the phi insertion
    // to propagate the values back to the currrent block. Also needs to insert the name into the local block revision
    // tables.
    std::pair<Value, Value> findName(Hash name);
    // Returns the local value number after insertion. May insert Phis recursively in all predecessors.
    Value findValue(Value v);
    Value findValuePredecessor(Value v, Block* block, std::unordered_map<int, Value>& blockValues);

    Lexer* m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    Frame* m_frame;
    Block* m_block;
    int m_blockSerial; // huh, what happens if blocks and values get same serial numbers?
    uint32_t m_valueSerial;
    bool m_changeMade;
};

} // namespace hadron

#endif // SRC_INCLUDE_HADRON_SSA_BUILDER_HPP_