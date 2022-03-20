#ifndef SRC_HADRON_BLOCK_HPP_
#define SRC_HADRON_BLOCK_HPP_

#include "hadron/hir/HIR.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/library/Kernel.hpp"
#include "hadron/library/Symbol.hpp"
#include "hadron/Scope.hpp"

#include <list>
#include <memory>
#include <unordered_map>

namespace hadron {

namespace hir {
struct AssignHIR;
}

struct Frame;

class Block {
public:
    using ID = int32_t;

    Block() = delete;
    Block(Scope* owningScope, ID blockID, bool isSealed = true);
    ~Block() = default;

    // Adds a statement to the end of the block.
    hir::ID append(std::unique_ptr<hir::HIR> hir);
    // Adds a statement right before the first branch or return instruction within the block, making |hir| the last
    // statement before block exit.
    hir::ID prependExit(std::unique_ptr<hir::HIR> hir);
    // Adds a statement to the top of the block.
    hir::ID prepend(std::unique_ptr<hir::HIR> hir);

    // Follow order of precedence in names to locate an identifer symbol, including in local variables, arguments,
    // instance variables, class variables, and pre-defined identifiers. Can return hir::kInvalidID, which means a
    // compilation error that the name is not found.
    hir::AssignHIR* findName(ThreadContext* context, library::Symbol name);

    // For unsealed blocks, resolves all incomplete phis and marks the block as sealed.
    void seal(ThreadContext* context);

    inline std::unordered_map<library::Symbol, hir::AssignHIR*>& nameAssignments() { return m_nameAssignments; }
    inline Scope* scope() const { return m_scope; }
    inline Frame* frame() const { return m_frame; }
    inline ID id() const { return m_id; }
    inline std::list<Block*>& predecessors() { return m_predecessors; }
    inline std::list<Block*>& successors() { return m_successors; }
    inline std::list<std::unique_ptr<hir::PhiHIR>>& phis() { return m_phis; }
    inline std::list<std::unique_ptr<hir::HIR>>& statements() { return m_statements; }
    inline bool hasMethodReturn() const { return m_hasMethodReturn; }
    inline bool isSealed() const { return m_isSealed; }
    inline hir::ID finalValue() const { return m_finalValue; }

    void setHasMethodReturn(bool hasReturn) { m_hasMethodReturn = hasReturn; }
    void setFinalValue(hir::ID value) { assert(value != hir::kInvalidID); m_finalValue = value; }

private:
    hir::ID insert(std::unique_ptr<hir::HIR> hir, std::list<std::unique_ptr<hir::HIR>>::iterator iter);

    // Recursively traverse through blocks looking for recent revisions of the value and type. Then do the phi insertion
    // to propagate the values back to the currrent block. Also needs to insert the name into the local block revision
    // tables. Can return nullptr which means the name was not found.
    hir::AssignHIR* findScopedName(ThreadContext* context, library::Symbol name);
    hir::AssignHIR* findScopedNameRecursive(ThreadContext* context, library::Symbol name,
                                              std::unordered_map<Block::ID, hir::AssignHIR*>& blockValues,
                                              const std::unordered_set<const Scope*>& containingScopes,
                                              std::unordered_map<hir::HIR*, hir::HIR*>& trivialPhis);

    // Any assignments of value ids to named values must occur with AssignHIR statements. These allow us to track
    // changes to named values that might need to be synchronized to the heap, as well as allowing the value id to be
    // manipulated like normal HIR value ids, such as during trivial phi deletion or constant folding.
    std::unordered_map<library::Symbol, hir::AssignHIR*> m_nameAssignments;

    // Owning scope of this block.
    Scope* m_scope;
    // The top-level frame that contains this block.
    Frame* m_frame;

    ID m_id;
    std::list<Block*> m_predecessors;
    std::list<Block*> m_successors;

    // Phis are conceptually all executed simultaneously at Block entry and so are maintained separately.
    std::list<std::unique_ptr<hir::PhiHIR>> m_phis;

    // Statements in order of execution.
    std::list<std::unique_ptr<hir::HIR>> m_statements;

    std::list<std::unique_ptr<hir::HIR>>::iterator m_prependExitIterator;

    bool m_hasMethodReturn;

    // Sealed blocks have had all their predecessors added, and so can complete phis. Unsealed blocks cannot, and so
    // we create incomplete phis and use them until the block can be sealed.
    bool m_isSealed;
    std::list<std::unique_ptr<hir::PhiHIR>> m_incompletePhis;

    hir::ID m_finalValue;
};

} // namespace hadron

#endif // SRC_HADRON_BLOCK_HPP_