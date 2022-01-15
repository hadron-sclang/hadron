#ifndef SRC_HADRON_BLOCK_HPP_
#define SRC_HADRON_BLOCK_HPP_

#include "hadron/HIR.hpp"

#include <memory>
#include <unordered_map>

namespace hadron {

struct Scope;

struct Block {
    Block() = delete;
    Block(Scope* owningScope, int blockNumber): scope(owningScope), number(blockNumber) {}
    ~Block() = default;

    // Value numbers are frame-wide but for LVN the value lookups are block-local, because extra-block values
    // need to go through a Phi function in this Block. For local value numbering we keep a map of the
    // value to the associated HIR instruction, for possible re-use of instructions.
    std::unordered_map<Value, hir::HIR*> values;
    // Map of names (variables, arguments) to most recent revision of <values, type>
    std::unordered_map<Hash, std::pair<Value, Value>> revisions;

    // Map of values defined extra-locally and their local value. For convenience we also put local values in here,
    // mapping to themselves.
    std::unordered_map<Value, Value> localValues;
    // Owning scope of this block.
    Scope* scope;
    // Unique block number.
    int number;
    std::list<Block*> predecessors;
    std::list<Block*> successors;

    std::list<std::unique_ptr<hir::PhiHIR>> phis;
    // Statements in order of execution.
    std::list<std::unique_ptr<hir::HIR>> statements;

    // The value of executing any block is the final value that was created in the block.
    std::pair<Value, Value> finalValue;
};

} // namespace hadron

#endif // SRC_HADRON_BLOCK_HPP_