#ifndef SRC_BLOCK_HPP_
#define SRC_BLOCK_HPP_

#include "HIR.hpp"
#include "Value.hpp"

#include <unordered_map>
#include <vector>

namespace hadron {

// Transform parse tree nodes into code blocks and instructions, for control flow analysis, subsequent transformation
// into SSA form, and finishing IR code generation.
// needs to have a symbol table for locally-scoped variables, or just could be a set of hashes to test membership
// can have children blocks for control flow
// has exactly one entry point and one exit point (with possibly multiple destinations)
struct Block {
    Block(int uniqueID): id(uniqueID) {}

    // To facilitate control flow graph traversal (which may be cyclic) each Block is given a number intended to be
    // unique within the graph this Block is in.
    int id;

    std::unordered_map<uint64_t, Value> values;
    // Entry point is assumed to be the first instruction, if any.
    std::vector<HIR> instructions;
    // This is for control flow graphs, the exits are in order depending on the last instruction. If no exits the
    // Block is assumed to return?
    std::vector<Block*> exits;

    // Blocks are connected in a tree for the purposes of scoping, and we make ownership follow this pattern as well,
    // because control flow graphs are not necessarily acyclic.
    Block* scopeParent = nullptr;
    std::vector<std::unique_ptr<Block>> scopeChildren;
};

} // namespace hadron

#endif // SRC_BLOCK_HPP_