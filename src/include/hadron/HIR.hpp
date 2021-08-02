#ifndef SRC_INCLUDE_HADRON_HIR_HPP_
#define SRC_INCLUDE_HADRON_HIR_HPP_

#include "hadron/Hash.hpp"

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

// The literature calls the entire data structure a Control Flow Graph, and the individual nodes are called Blocks.
// There is also a strict heirarchy of scope, which the LSC parser at least calls a Block. Could be called a Frame,
// since it will most definitely correspond to a stack frame. Frames are where local (read: stack-based) variables
// live, and Frames are nestable.

// So the plan is to follow Braun 2013 to build the SSA directly from the parse tree, skipping AST construction.
// This takes a few passes to clean up and get to minimal pruned SSA form. Type computations follow along for the
// ride as parallel variables to their named values. We perform LVN and constant folding as we go.

// We can then do a bottoms-up dead code elimination pass. This allows us to be verbose with the type system
// assignments, I think, as we can treat them like parallel values.
namespace hadron {

struct Block;

namespace hir {

enum Opcode {
    kLoadArgument,
    kConstant,
    kCopy,
    kBinop,
    kDispatch,
    kPsi
};

struct Reference {
    Hash name;
    int32_t revision;
    Block* block;
    HIR* statement;
};

// All HIR instructions modify the value, thus creating a new version, and may read multiple other values.
struct HIR {
    HIR() = delete;
    explicit HIR(Opcode op, Hash tgt): opcode(op), target(tgt), revision(-1) {}
    Opcode opcode;
    Hash target;
    int32_t revision;
};

struct LoadArgumentHIR : public HIR {
    LoadArgumentHIR(Hash tgt, int32_t argIndex): HIR(kLoadArgument, tgt), index(argIndex) {}
    int32_t index;
};

} // namespace hir



} // namespace hadron

#endif  // SRC_INCLUDE_HADRON_HIR_HPP_