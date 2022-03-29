#include "hadron/Block.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/Frame.hpp"
#include "hadron/hir/BlockLiteralHIR.hpp"
#include "hadron/hir/ConstantHIR.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/hir/RouteToSuperclassHIR.hpp"
#include "hadron/library/Thread.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include <spdlog/spdlog.h>

#include <stack>

namespace hadron {

Block::Block(Scope* owningScope, Block::ID blockID):
        m_scope(owningScope),
        m_frame(owningScope->frame),
        m_id(blockID),
        m_prependExitIterator(m_statements.end()),
        m_hasMethodReturn(false),
        m_finalValue(hir::kInvalidID) {}

hir::ID Block::append(std::unique_ptr<hir::HIR> hir) {
    return insert(std::move(hir), m_statements.end());
}

hir::ID Block::prependExit(std::unique_ptr<hir::HIR> hir) {
    return insert(std::move(hir), m_prependExitIterator);
}

hir::ID Block::prepend(std::unique_ptr<hir::HIR> hir) {
    return insert(std::move(hir), m_statements.begin());
}

hir::ID Block::insert(std::unique_ptr<hir::HIR> hir, std::list<std::unique_ptr<hir::HIR>>::iterator iter) {
    // Phis should only be inserted by findScopedName().
    assert(hir->opcode != hir::Opcode::kPhi);

    // Re-use constants with the same values.
    if (hir->opcode == hir::Opcode::kConstant) {
        // We're possibly skipping dependency updates for this constant, so ensure that constants never have value
        // dependencies.
        assert(hir->reads.size() == 0);
        auto constantHIR = reinterpret_cast<hir::ConstantHIR*>(hir.get());
        auto constantIter = m_frame->constantValues.find(constantHIR->constant);
        if (constantIter != m_frame->constantValues.end()) {
            return constantIter->second;
        }
    }

    auto valueNumber = static_cast<hir::ID>(m_frame->values.size());
    auto value = hir->proposeValue(valueNumber);
    // We don't bump the value serial for invalid values (meaning read-only operations)
    if (value != hir::kInvalidID) {
        m_frame->values.emplace_back(hir.get());
    }

    hir->owningBlock = this;

    // Update the producers of values this hir consumes.
    for (auto id : hir->reads) {
        assert(m_frame->values[id]);
        m_frame->values[id]->consumers.insert(hir.get());
    }

    bool keepPrependExitIter = false;

    // Adding a new constant, update the constants map and set.
    if (hir->opcode == hir::Opcode::kConstant) {
        auto constantHIR = reinterpret_cast<hir::ConstantHIR*>(hir.get());
        m_frame->constantValues.emplace(std::make_pair(constantHIR->constant, value));
        m_frame->constantIds.emplace(value);
    } else if (hir->opcode == hir::Opcode::kMethodReturn) {
        m_hasMethodReturn = true;
        keepPrependExitIter = true;
    } else if (hir->opcode == hir::Opcode::kBranch || hir->opcode == hir::Opcode::kBranchIfTrue) {
        keepPrependExitIter = true;
    }

    auto emplaceIter = m_statements.emplace(iter, std::move(hir));

    if (keepPrependExitIter && m_prependExitIterator == m_statements.end()) {
        m_prependExitIterator = emplaceIter;
    }

    return value;
}

} // namespace hadron