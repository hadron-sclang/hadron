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

struct Frame;

class Block {
public:
    using ID = int32_t;

    Block() = delete;
    Block(Scope* owningScope, ID blockID);
    ~Block() = default;

    // Adds a statement to the end of the block.
    hir::ID append(std::unique_ptr<hir::HIR> hir);
    // Adds a statement right before the first branch or return instruction within the block, making |hir| the last
    // statement before block exit.
    hir::ID prependExit(std::unique_ptr<hir::HIR> hir);
    // Adds a statement to the top of the block.
    hir::ID prepend(std::unique_ptr<hir::HIR> hir);

    inline Scope* scope() const { return m_scope; }
    inline Frame* frame() const { return m_frame; }
    inline ID id() const { return m_id; }
    inline std::list<Block*>& predecessors() { return m_predecessors; }
    inline std::list<Block*>& successors() { return m_successors; }
    inline std::list<std::unique_ptr<hir::PhiHIR>>& phis() { return m_phis; }
    inline std::list<std::unique_ptr<hir::HIR>>& statements() { return m_statements; }
    inline bool hasMethodReturn() const { return m_hasMethodReturn; }
    inline hir::ID finalValue() const { return m_finalValue; }

    void setHasMethodReturn(bool hasReturn) { m_hasMethodReturn = hasReturn; }
    void setFinalValue(hir::ID value) { assert(value != hir::kInvalidID); m_finalValue = value; }

private:
    hir::ID insert(std::unique_ptr<hir::HIR> hir, std::list<std::unique_ptr<hir::HIR>>::iterator iter);

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

    hir::ID m_finalValue;
};

} // namespace hadron

#endif // SRC_HADRON_BLOCK_HPP_