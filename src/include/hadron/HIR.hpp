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
// ride as parallel variables to their names values. We perform LVN and constant folding as we go.

// We can then do a bottoms-up dead code elimination pass. This allows us to be verbose with the type system
// assignments, I think, 
namespace hadron {

namespace hir {


// We squish everything into a top-level scope
// value names are going to be _<name>_<owningBlockNo>_value
// or _<name>_<owningBlockNo>_type ...
struct BlockHIR {
    int blockNumber;
    std::unordered_map<Hash, std::string> valueNames;
    std::list<std::unique_ptr<StatementHIR>> statements;
};

struct ValueHIR {
    int owningBlock;
    Hash nameHash;
    int revision;
};

// TODO: It is time to unify Literal, ConstantAST, etc
struct ConstantHIR {

};

enum OpcodeHIR {
    kLoadArgument, // modifies the target
    kAssign,       // modifies the target
    kBinop,        // can be assigned to something
    kDispatch,     // reads many values, has to be assumed to modify the target
    kPsi,
    kBranch,

};

struct StatementHIR {
    Opc
};


} // namespace hir

} // namespace hadron

#endif  // SRC_INCLUDE_HADRON_HIR_HPP_