#ifndef SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_
#define SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_

#include "hadron/Hash.hpp"
#include "hadron/HIR.hpp"
#include "hadron/Slot.hpp"

#include "hadron/library/Symbol.hpp"

#include <memory>

namespace hadron {

struct Block;
class ErrorReporter;
struct Frame;
class Lexer;
struct Scope;
struct ThreadContext;

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
    BlockBuilder() = delete;
    BlockBuilder(const Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter);
    ~BlockBuilder();

    std::unique_ptr<Frame> buildFrame(ThreadContext* context, const parse::BlockNode* blockNode);

private:
    // Scopes must have exactly one entry block that has exactly one predecessor.
    std::unique_ptr<Scope> buildSubScope(ThreadContext* context, const parse::BlockNode* blockNode);

    // Take the expression sequence in |node|, build SSA form out of it, return pair of value numbers associated with
    // expression value and expression type respectively. While it will process all descendents of |node| it will not
    // iterate to process the |node->next| pointer. Call buildFinalValue() to do that.
    std::pair<Value, Value> buildValue(ThreadContext* context, const parse::Node* node);
    std::pair<Value, Value> buildFinalValue(ThreadContext* context, const parse::Node* node);
    std::pair<Value, Value> buildDispatch(ThreadContext* context, const parse::Node* target, library::Symbol selector,
            const parse::Node* arguments, const parse::KeyValueNode* keywordArguments);
    // It's also possible to build a dispatch with already evaluated values (that may not exist in the parse tree).
    // This function expects the target as first value pair in |argumentValues|.
    std::pair<Value, Value> buildDispatchInternal(std::pair<Value, Value> selectorValues,
            const std::vector<std::pair<Value, Value>>& argumentValues,
            const std::vector<std::pair<Value, Value>>& keywordArgumentValues);

    // Returns the value either inserted or re-used (if a constant). Takes ownership of hir.
    Value insertLocal(std::unique_ptr<hir::HIR> hir);
    Value insert(std::unique_ptr<hir::HIR> hir, Block* block);

    // Recursively traverse through blocks looking for recent revisions of the value and type. Then do the phi insertion
    // to propagate the values back to the currrent block. Also needs to insert the name into the local block revision
    // tables.
    std::pair<Value, Value> findName(Hash name);
    // Recursive traversal up the Block graph looking for a prior definition of the provided name.
    std::pair<Value, Value> findNamePredecessor(Hash name, Block* block,
            std::unordered_map<int, std::pair<Value, Value>>& blockValues,
            const std::unordered_set<const Scope*>& containingScopes);

    // Returns the local value number after insertion. May insert Phis recursively in all predecessors.
    Value findValue(Value v);
    Value findValuePredecessor(Value v, Block* block, std::unordered_map<int, Value>& blockValues);

    const Lexer* m_lexer;
    std::shared_ptr<ErrorReporter> m_errorReporter;
    Frame* m_frame;
    Scope* m_scope;
    Block* m_block;
    int m_blockSerial;
    uint32_t m_valueSerial;
};

} // namespace hadron

#endif // SRC_COMPILER_INCLUDE_HADRON_BLOCK_BUILDER_HPP_