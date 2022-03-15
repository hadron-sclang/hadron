#ifndef SRC_HADRON_BLOCK_HPP_
#define SRC_HADRON_BLOCK_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/Scope.hpp"

#include <memory>
#include <unordered_map>

namespace hadron {

namespace hir {
struct AssignHIR;
}

struct Frame;

struct Block {
    using ID = int32_t;

    Block() = delete;
    Block(Scope* owningScope, ID blockID):
            scope(owningScope),
            frame(owningScope->frame),
            id(blockID),
            finalValue(hir::kInvalidID),
            hasMethodReturn(false),
            isSealed(true) {}
    ~Block() = default;

    // Any assignments of value ids to named values must occur with AssignHIR statements. These allow us to track
    // changes to named values that might need to be synchronized to the heap, as well as allowing the value id to be
    // manipulated like normal HIR value ids, such as during trivial phi deletion or constant folding.
    std::unordered_map<library::Symbol, hir::AssignHIR*> assignments;

    // Owning scope of this block.
    Scope* scope;
    // The top-level frame that contains this block.
    Frame* frame;

    ID id;
    std::list<Block*> predecessors;
    std::list<Block*> successors;

    // Phis are conceptually all executed simultaneously at Block entry and so are maintained separately.
    std::list<std::unique_ptr<hir::PhiHIR>> phis;

    // Statements in order of execution.
    std::list<std::unique_ptr<hir::HIR>> statements;

    // The value of executing any block is the final value that was created in the block.
    hir::ID finalValue;
    bool hasMethodReturn;

    // Sealed blocks have had all their predecessors added, and so can complete phis. Unsealed blocks cannot, and so
    // we create incomplete phis and use them until the block can be sealed.
    bool isSealed;
    std::list<std::unique_ptr<hir::PhiHIR>> incompletePhis;
};

} // namespace hadron

#endif // SRC_HADRON_BLOCK_HPP_