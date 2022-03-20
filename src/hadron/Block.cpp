#include "hadron/Block.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/hir/AssignHIR.hpp"
#include "hadron/hir/BlockLiteralHIR.hpp"
#include "hadron/hir/ConstantHIR.hpp"
#include "hadron/hir/ImportClassVariableHIR.hpp"
#include "hadron/hir/ImportInstanceVariableHIR.hpp"
#include "hadron/hir/ImportLocalVariableHIR.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/hir/RouteToSuperclassHIR.hpp"
#include "hadron/Frame.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include <spdlog/spdlog.h>

#include <stack>

namespace hadron {

Block::Block(Scope* owningScope, Block::ID blockID, bool isSealed):
        m_scope(owningScope),
        m_frame(owningScope->frame),
        m_id(blockID),
        m_prependExitIterator(m_statements.end()),
        m_hasMethodReturn(false),
        m_isSealed(isSealed),
        m_finalValue(hir::kInvalidID) {}

hir::ID Block::append(std::unique_ptr<hir::HIR> hir) {
    auto value = insert(std::move(hir), m_statements.end());
    if (value != hir::kInvalidID) {
        m_finalValue = value;
    }
    return value;
}

hir::ID Block::prependExit(std::unique_ptr<hir::HIR> hir) {
    auto value = insert(std::move(hir), m_prependExitIterator);
    if (value != hir::kInvalidID) {
        m_finalValue = value;
    }
    return value;
}

hir::ID Block::prepend(std::unique_ptr<hir::HIR> hir) {
    auto value = insert(std::move(hir), m_statements.begin());
    if (m_finalValue == hir::kInvalidID) {
        m_finalValue = value;
    }
    return value;
}

hir::AssignHIR* Block::findName(ThreadContext* context, library::Symbol name) {
    // Check local block cache first in case a value is already cached.
    auto assignIter = m_nameAssignments.find(name);
    if (assignIter != m_nameAssignments.end()) {
        return assignIter->second;
    }

    // If this symbol defines a class name look it up in the class library and provide it as a constant.
    if (name.isClassName(context)) {
        auto classDef = context->classLibrary->findClassNamed(name);
        assert(!classDef.isNil());
        auto constant = std::make_unique<hir::ConstantHIR>(classDef.slot());
        hir::ID nodeValue = append(std::move(constant));
        auto assignHIROwning = std::make_unique<hir::AssignHIR>(name, nodeValue);
        auto assignHIR = assignHIROwning.get();
        append(std::move(assignHIROwning));
        m_nameAssignments.emplace(std::make_pair(name, assignHIR));
        return assignHIR;
    }

    // Search through local values, including variables, arguments, and already-cached imports, for a name.
    Block* innerBlock = this;
    hir::BlockLiteralHIR* outerBlockHIR = m_frame->outerBlockHIR;

    std::stack<Block*> innerBlocks;
    std::stack<hir::BlockLiteralHIR*> outerBlockHIRs;

    while (innerBlock) {
        auto assignHIR = innerBlock->findScopedName(context, name);

        if (assignHIR) {
            // If we found this value in a external frame we will need to add import statements to each frame between
            // this block and the importing block, starting with the outermost frame that didn't have the value.
            while (outerBlockHIRs.size()) {
                // At start of loop innerBlock is pointing to the block where the value was found and outerBlockHIR is
                // pointing to the outer block that already contains the value, so pop from the stack to point it at
                // the top BlockLiteralHIR that needs to have import statements prepared.
                outerBlockHIR = outerBlockHIRs.top();
                outerBlockHIRs.pop();

                // **Note** that right now innerBlock is actually pointing at the block *containing* outerBlockHIR.
                assert(outerBlockHIR->owningBlock == innerBlock);

                // At the start of the loop assignHIR has the value of name within outerBlockHIR's owning block. Add
                // appropriate reads and consumer pointers so that any import statements can also be modified during
                // outer block value replacements.
                outerBlockHIR->reads.emplace(assignHIR->valueId);
                auto producerHIR = innerBlock->m_frame->values[assignHIR->valueId];
                assert(producerHIR);
                producerHIR->consumers.emplace(outerBlockHIR);

                // Now we need to add import statements to the import block inside the frame owned by outerBlockHIR.
                auto import = std::make_unique<hir::ImportLocalVariableHIR>(producerHIR->typeFlags, assignHIR->valueId);

                // Now innerBlock should point to the block containing either the current search or the next nested
                // BlockLiteralHIR.
                innerBlock = innerBlocks.top();
                innerBlocks.pop();

                // Insert just before the branch statement at the end of the import block.
                auto importBlock = innerBlock->m_frame->rootScope->blocks.front().get();
                auto importValue = importBlock->prependExit(std::move(import));

                auto assignHIROwning = std::make_unique<hir::AssignHIR>(name, importValue);
                importBlock->m_nameAssignments.emplace(std::make_pair(name, assignHIROwning.get()));
                importBlock->prependExit(std::move(assignHIROwning));

                // Now plumb that value through to the block containing the inner frame, which might be a different
                // value if there are incomplete phis between import and inner blocks.
                assignHIR = innerBlock->findScopedName(context, name);
            }

            assert(innerBlock == this);
            return assignHIR;
        }

        if (outerBlockHIR) {
            innerBlocks.emplace(innerBlock);
            innerBlock = outerBlockHIR->owningBlock;

            outerBlockHIRs.emplace(outerBlockHIR);
            outerBlockHIR = innerBlock->m_frame->outerBlockHIR;
        } else {
            innerBlock = nullptr;
        }
    }

    // The next several options will all require an import, so set up the iterator for HIR insertion to point at the
    // branch instruction in the import block, so that the imported HIR will insert right before it.
    hir::ID nodeValue = hir::kInvalidID;
    Block* importBlock = m_frame->rootScope->blocks.front().get();

    // Search instance variables next.
    library::Class classDef = m_frame->method.ownerClass();
    auto className = classDef.name(context).view(context);
    bool isMetaClass = (className.size() > 5) && (className.substr(0, 5) == "Meta_");

    // Meta_ classes are descended from Class, so don't have access to these regular instance variables, so skip the
    // search for instance variable names in that case.
    if (!isMetaClass) {
        auto instVarIndex = classDef.instVarNames().indexOf(name);
        if (instVarIndex.isInt32()) {
            auto thisAssign = findName(context, context->symbolTable->thisSymbol());
            // *this* should always be provided as a frame-level argument, so should always resolve.
            assert(thisAssign);
            nodeValue = importBlock->prependExit(std::make_unique<hir::ImportInstanceVariableHIR>(thisAssign->valueId,
                    instVarIndex.getInt32()));
        }
    } else {
        // Meta_ classes to have access to the class variables and constants of their associated class, so adjust the
        // classDef to point to the associated class instead.
        className = className.substr(5);
        classDef = context->classLibrary->findClassNamed(library::Symbol::fromView(context, className));
        assert(!classDef.isNil());
    }

    // Search class variables next, starting from this class and up through all parents.
    if (nodeValue == hir::kInvalidID) {
        library::Class classVarDef = classDef;

        while (!classVarDef.isNil()) {
            auto classVarOffset = classVarDef.classVarNames().indexOf(name);
            if (classVarOffset.isInt32()) {
                nodeValue = importBlock->prependExit(std::make_unique<hir::ImportClassVariableHIR>(classVarDef,
                        classVarOffset.getInt32()));
                break;
            }

            classVarDef = context->classLibrary->findClassNamed(classVarDef.superclass(context));
        }
    }

    // Search constants next.
    if (nodeValue == hir::kInvalidID) {
        library::Class classConstDef = classDef;

        while (!classConstDef.isNil()) {
            auto constIndex = classConstDef.constNames().indexOf(name);
            if (constIndex.isInt32()) {
                // We still put constants in the import block to avoid them being undefined along any path in the CFG.
                nodeValue = importBlock->prependExit(std::make_unique<hir::ConstantHIR>(
                        classDef.constValues().at(constIndex.getInt32())));
                break;
            }

            classConstDef = context->classLibrary->findClassNamed(classConstDef.superclass(context));
        }
    }

    // If we found a match we've inserted it into the import block. Use findScopedName to set up all the phis
    // and local value mappings between the import block and the current block.
    if (nodeValue != hir::kInvalidID) {
        auto assignHIR = std::make_unique<hir::AssignHIR>(name, nodeValue);
        importBlock->m_nameAssignments[name] = assignHIR.get();
        importBlock->prependExit(std::move(assignHIR));

        return findScopedName(context, name);
    }

    // Check for special names.
    if (name == context->symbolTable->superSymbol()) {
        auto thisAssign = findName(context, context->symbolTable->thisSymbol());
        assert(thisAssign);
        nodeValue = append(std::make_unique<hir::RouteToSuperclassHIR>(thisAssign->valueId));
    } else if (name == context->symbolTable->thisMethodSymbol()) {
        nodeValue = append(std::make_unique<hir::ConstantHIR>(m_frame->method.slot()));
    } else if (name == context->symbolTable->thisProcessSymbol()) {
        nodeValue = append(std::make_unique<hir::ConstantHIR>(Slot::makePointer(context->thisProcess)));
    } else if (name == context->symbolTable->thisThreadSymbol()) {
        nodeValue = append(std::make_unique<hir::ConstantHIR>(Slot::makePointer(context->thisThread)));
    }

    // Special names can all be appended locally to the block, no import required
    if (nodeValue != hir::kInvalidID) {
        auto assignHIROwning = std::make_unique<hir::AssignHIR>(name, nodeValue);
        auto assignHIR = assignHIROwning.get();
        m_nameAssignments[name] = assignHIR;
        return assignHIR;
    } else {
        SPDLOG_CRITICAL("Failed to find name: {}", name.view(context));
    }

    return nullptr;
}

void Block::seal(ThreadContext* context) {
    assert(!m_isSealed);
    m_isSealed = true;

    std::unordered_map<hir::HIR*, hir::HIR*> trivialPhis;

    // Resolve each phi by looking in each predecessor for the name of the phi.
    for (auto& phi : m_incompletePhis) {
        for (auto pred : m_predecessors) {
            auto predValue = pred->findName(context, phi->name);

            assert(predValue);
            assert(m_frame->values[predValue->valueId]);
            phi->addInput(m_frame->values[predValue->valueId]);
        }

        auto trivialValue = phi->getTrivialValue();
        if (trivialValue != hir::kInvalidID) {
            assert(m_frame->values[trivialValue]);
            SPDLOG_INFO("sealing rendered {} trivial, replacing with {}", phi->id, m_frame->values[trivialValue]->id);
            trivialPhis.emplace(std::make_pair(phi.get(), m_frame->values[trivialValue]));
        }
    }

    // Add all phis to the official phi list.
    m_phis.splice(m_phis.end(), m_incompletePhis);

    // Now clean up any of the trivial phis that we may have detected.
    m_frame->replaceValues(trivialPhis);
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

hir::AssignHIR* Block::findScopedName(ThreadContext* context, library::Symbol name) {
    std::unordered_map<int32_t, hir::AssignHIR*> blockValues;

    std::unordered_set<const Scope*> containingScopes;
    const Scope* scope = m_scope;
    while (scope) {
        containingScopes.emplace(scope);
        scope = scope->parent;
    }

    std::unordered_map<hir::HIR*, hir::HIR*> trivialPhis;

    auto assignHIR = findScopedNameRecursive(context, name, blockValues, containingScopes, trivialPhis);
    if (!assignHIR) { return nullptr; }

    while (trivialPhis.size()) {
        m_frame->replaceValues(trivialPhis);
        trivialPhis.clear();
        blockValues.clear();
        assignHIR = findScopedNameRecursive(context, name, blockValues, containingScopes, trivialPhis);
        assert(assignHIR);
    }

    return assignHIR;
}

hir::AssignHIR* Block::findScopedNameRecursive(ThreadContext* context, library::Symbol name,
        std::unordered_map<Block::ID, hir::AssignHIR*>& blockValues,
        const std::unordered_set<const Scope*>& containingScopes,
        std::unordered_map<hir::HIR*, hir::HIR*>& trivialPhis) {

    // To avoid any infinite cycles in block loops, return any value found on a previous call on this block.
    auto cacheIter = blockValues.find(m_id);
    if (cacheIter != blockValues.end()) {
        return cacheIter->second;
    }

    // This scope is *shadowing* the variable name if the scope has a variable of the same name and is not within
    // the scope heirarchy of the search, so we should ignore any local revisions.
    bool isShadowed = (m_scope->variableNames.count(name) > 0) && (containingScopes.count(m_scope) == 0);
    assert(!isShadowed);

    auto iter = m_nameAssignments.find(name);
    if (iter != m_nameAssignments.end()) {
        if (!isShadowed) {
            SPDLOG_INFO("revisions hit for {} in block {} with value {}", name.view(context), m_id,
                    iter->second->valueId);
            return iter->second;
        }
    }

    // Unsealed blocks always create phis, because we can't search the complete list of predecessors.
    if (m_isSealed) {
        auto phi = std::make_unique<hir::PhiHIR>(name);
        // This is an empty phi until the block is sealed, so we set its type widely for now, can refine type of the phi
        // once the block is sealed.
        phi->typeFlags = TypeFlags::kAllFlags;
        phi->owningBlock = this;
        auto phiValue = phi->proposeValue(static_cast<hir::ID>(m_frame->values.size()));
        m_frame->values.emplace_back(phi.get());
        m_incompletePhis.emplace_back(std::move(phi));

        auto assignHIROwning = std::make_unique<hir::AssignHIR>(name, phiValue);
        auto assignHIR = assignHIROwning.get();
        prepend(std::move(assignHIROwning));
        m_nameAssignments[name] = assignHIR;
        return assignHIR;
    }

    // Don't bother with phi creation if we have no or only one predecessor.
    if (m_predecessors.size() == 0) {
        return nullptr;
    } else if (m_predecessors.size() == 1) {
        auto foundValue = m_predecessors.front()->findScopedNameRecursive(context, name, blockValues, containingScopes,
                trivialPhis);
        return foundValue;
    }

    // Either no local revision found or the local revision is ignored, we need to search recursively upward. Create
    // a phi for possible insertion into the local map (if not shadowed).
    auto phi = std::make_unique<hir::PhiHIR>(name);
    auto phiValue = phi->proposeValue(static_cast<hir::ID>(m_frame->values.size()));
    phi->owningBlock = this;
    m_frame->values.emplace_back(phi.get());
    auto assignHIR = std::make_unique<hir::AssignHIR>(name, phiValue);
    blockValues.emplace(std::make_pair(m_id, assignHIR.get()));

    // Search predecessors for the name.
    for (auto pred : m_predecessors) {
        auto foundAssign = pred->findScopedNameRecursive(context, name, blockValues, containingScopes, trivialPhis);

        // This is a depth-first search, so the above recursive call will return after searching up until either the
        // name is found or the import block (with no predecessors) is found empty. So an invalidID return here means
        // we have not found the name in any scope along the path from here to root, and need to clean up the phi and
        // return nullptr
        if (!foundAssign) {
            assert(phi->reads.size() == 0);
            assert(trivialPhis.size() == 0);
            m_frame->values[phiValue] = nullptr;
            blockValues[m_id] = nullptr;
            return nullptr;
        }

        assert(m_frame->values[foundAssign->valueId]);
        phi->addInput(m_frame->values[foundAssign->valueId]);
    }
    assert(phi->inputs.size());

    // If trivial, add it to the list of values to replace with its trivial value.
    auto trivialValue = phi->getTrivialValue();
    if (trivialValue != hir::kInvalidID) {
        SPDLOG_INFO("{} trivial phi, replace {} with {} in block {}", name.view(context), phiValue, trivialValue, m_id);
        assert(m_frame->values[trivialValue]);
        trivialPhis.emplace(std::make_pair(phi.get(), m_frame->values[trivialValue]));
    }

    auto assignRef = assignHIR.get();
    m_nameAssignments[name] = assignRef;
    prepend(std::move(assignHIR));
    m_phis.emplace_back(std::move(phi));

    return assignRef;
}


} // namespace hadron