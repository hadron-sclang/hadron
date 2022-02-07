#ifndef SRC_HADRON_BLOCK_HPP_
#define SRC_HADRON_BLOCK_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/library/Symbol.hpp"

#include <memory>
#include <unordered_map>

namespace hadron {

struct Frame;
struct Scope;

struct Block {
    Block() = delete;
    Block(Scope* owningScope, int blockNumber): scope(owningScope), frame(owningScope->frame), number(blockNumber) {}
    ~Block() = default;

    // Map of names to most recent revision of local values.
    std::unordered_map<library::Symbol, hir::NVID> revisions;

    // Map of values defined outside of the block and their local value. For convenience we also put local values
    // in here, mapping to themselves.
    std::unordered_map<hir::NVID, hir::NVID> localValues;
    // Owning scope of this block.
    Scope* scope;
    // The top-level frame that contains this block.
    Frame* frame;

    // Unique block number.
    int32_t number;
    std::list<Block*> predecessors;
    std::list<Block*> successors;

    // Phis are conceptually all executed simultaneously at Block entry and so are maintained separately.
    std::list<std::unique_ptr<hir::PhiHIR>> phis;

    // Statements in order of execution.
    std::list<std::unique_ptr<hir::HIR>> statements;

    // The value of executing any block is the final value that was created in the block.
    hir::NVID finalValue;
};

} // namespace hadron

#endif // SRC_HADRON_BLOCK_HPP_