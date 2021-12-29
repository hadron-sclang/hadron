#ifndef SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Slot.hpp"

#include <memory>
#include <vector>

namespace hadron {

struct Block;
class ErrorReporter;
struct Frame;
class Lexer;

namespace parse {
struct BlockNode;
struct KeyValueNode;
struct Node;
} // namespace parse

// Goes from parse tree to a Control Flow Graph of Blocks of HIR code in SSA form.
//
// This is an implementation of the algorithm described in [SSA2] "Simple and Efficient Construction of Static Single
// Assignment Form" by Bruan M. et al, with modifications to support type deduction while building SSA form.
class BlockBuilder {
public:
    BlockBuilder(const Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter);
    ~BlockBuilder();

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

    const Lexer* m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    Frame* m_frame;
    Block* m_block;
    int m_blockSerial; // huh, what happens if blocks and values get same serial numbers?
    uint32_t m_valueSerial;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_