#include "hadron/BlockBuilder.hpp"

#include "hadron/AST.hpp"
#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Heap.hpp"
#include "hadron/hir/BlockLiteralHIR.hpp"
#include "hadron/hir/BranchHIR.hpp"
#include "hadron/hir/BranchIfTrueHIR.hpp"
#include "hadron/hir/ConstantHIR.hpp"
#include "hadron/hir/HIR.hpp"
#include "hadron/hir/LoadArgumentHIR.hpp"
#include "hadron/hir/LoadFromPointerHIR.hpp"
#include "hadron/hir/LoadInstanceVariableHIR.hpp"
#include "hadron/hir/MessageHIR.hpp"
#include "hadron/hir/MethodReturnHIR.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/hir/StoreToPointerHIR.hpp"
#include "hadron/hir/StoreInstanceVariableHIR.hpp"
#include "hadron/hir/StoreReturnHIR.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/Scope.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <cassert>
#include <string>

namespace hadron {

BlockBuilder::BlockBuilder(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter) { }

BlockBuilder::~BlockBuilder() { }

std::unique_ptr<Frame> BlockBuilder::buildMethod(ThreadContext* context, const library::Method method,
        const ast::BlockAST* blockAST) {
    return buildFrame(context, method, blockAST);
}

std::unique_ptr<Frame> BlockBuilder::buildFrame(ThreadContext* context, const library::Method method,
    const ast::BlockAST* blockAST) {
    // Build outer frame, root scope, and entry block.
    auto frame = std::make_unique<Frame>();

    frame->argumentOrder = blockAST->argumentNames;
    frame->argumentDefaults = blockAST->argumentDefaults;

    frame->rootScope = std::make_unique<Scope>(frame.get());
    auto scope = frame->rootScope.get();
    scope->blocks.emplace_back(std::make_unique<Block>(scope, frame->numberOfBlocks));
    ++frame->numberOfBlocks;
    auto block = scope->blocks.front().get();

    // Load all arguments here.
    for (int32_t argIndex = 0; argIndex < frame->argumentOrder.size(); ++argIndex) {
        auto name = frame->argumentOrder.at(argIndex);
        auto loadArg = std::make_unique<hir::LoadArgumentHIR>(argIndex, name);

        // Set known class and type to *this* pointer.
        if (argIndex == 0) {
            loadArg->value.typeFlags = TypeFlags::kObjectFlag;
        } else if (blockAST->hasVarArg && argIndex == frame->argumentOrder.size() - 1) {
            // Variable arguments always have Array type.
            loadArg->value.typeFlags = TypeFlags::kObjectFlag;
            loadArg->value.knownClassName = library::Symbol::fromView(context, "Array");
        }

        block->revisions.emplace(std::make_pair(name, insert(std::move(loadArg), block)));
    }

    Block* currentBlock = block;
    currentBlock->finalValue = buildFinalValue(context, method, currentBlock, blockAST->statements.get());

     // We append a return statement in the final block, if one wasn't already provided.
    if (!currentBlock->hasMethodReturn) {
        insert(std::make_unique<hir::MethodReturnHIR>(), currentBlock);
        currentBlock->hasMethodReturn = true;
    }

    return frame;
}

std::unique_ptr<Scope> BlockBuilder::buildInlineBlock(ThreadContext* context, const library::Method method,
        Block* predecessor, const ast::BlockAST* blockAST) {
    auto scope = std::make_unique<Scope>(predecessor->scope);
    scope->blocks.emplace_back(std::make_unique<Block>(scope.get(), scope->frame->numberOfBlocks));
    ++scope->frame->numberOfBlocks;
    auto block = scope->blocks.front().get();
    block->predecessors.emplace_back(predecessor);

    // Inline blocks can have arguments but they are treated as constants with their default values. We skip the
    // *this* pointer for inline blocks so it doesn't shadow the frame level this pointer.
    for (int32_t argIndex = 1; argIndex < blockAST->argumentNames.size(); ++argIndex) {
        auto name = blockAST->argumentNames.at(argIndex);
        auto value = blockAST->argumentDefaults.at(argIndex);
        block->revisions.emplace(std::make_pair(name,
                insert(std::make_unique<hir::ConstantHIR>(value, name), block)));
    }

    Block* currentBlock = block;
    currentBlock->finalValue = buildFinalValue(context, method, currentBlock, blockAST->statements.get());
    return scope;
}

hir::NVID BlockBuilder::buildValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const ast::AST* ast) {
    hir::NVID nodeValue;

    switch(ast->astType) {
    case ast::ASTType::kEmpty:
        nodeValue = insert(std::make_unique<hir::ConstantHIR>(Slot::makeNil()), currentBlock);
        break;

    case ast::ASTType::kSequence: {
        const auto seq = reinterpret_cast<const ast::SequenceAST*>(ast);
        nodeValue = buildFinalValue(context, method, currentBlock, seq);
    } break;

    // Blocks encountered here are are block literals, and are candidates for inlining.
    case ast::ASTType::kBlock: {
        const auto blockAST = reinterpret_cast<const ast::BlockAST*>(ast);
        auto blockHIR = std::make_unique<hir::BlockLiteralHIR>(currentBlock->frame);
        blockHIR->frame = buildFrame(context, method, blockAST);
        nodeValue = insert(std::move(blockHIR), currentBlock);
    } break;

    case ast::ASTType::kIf: {
        const auto ifAST = reinterpret_cast<const ast::IfAST*>(ast);
        nodeValue = buildIf(context, method, currentBlock, ifAST);
    } break;

    case ast::ASTType::kMessage: {
        const auto message = reinterpret_cast<const ast::MessageAST*>(ast);

        auto messageHIR = std::make_unique<hir::MessageHIR>();
        messageHIR->selector = message->selector;

        // Build arguments.
        messageHIR->arguments.reserve(message->arguments->sequence.size());
        for (const auto& arg : message->arguments->sequence) {
            messageHIR->arguments.emplace_back(buildValue(context, method, currentBlock, arg.get()));
        }

        // Build keyword argument pairs.
        messageHIR->keywordArguments.reserve(message->keywordArguments->sequence.size());
        for (const auto& arg : message->keywordArguments->sequence) {
            messageHIR->keywordArguments.emplace_back(buildValue(context, method, currentBlock, arg.get()));
        }

        nodeValue = insert(std::move(messageHIR), currentBlock);
    } break;

    case ast::ASTType::kName: {
        const auto nameAST = reinterpret_cast<const ast::NameAST*>(ast);
        nodeValue = findName(context, method, nameAST->name, currentBlock);
    } break;

    case ast::ASTType::kAssign: {
        const auto assign = reinterpret_cast<const ast::AssignAST*>(ast);
        nodeValue = buildValue(context, method, currentBlock, assign->value.get());
        // TODO: StoreInstanceVariable? StoreClassVariable?
        currentBlock->revisions[assign->name->name] = nodeValue;
    } break;

    case ast::ASTType::kConstant: {
        const auto constAST = reinterpret_cast<const ast::ConstantAST*>(ast);
        nodeValue = insert(std::make_unique<hir::ConstantHIR>(constAST->constant), currentBlock);
    } break;

    case ast::ASTType::kMethodReturn: {
        const auto retAST = reinterpret_cast<const ast::MethodReturnAST*>(ast);
        nodeValue = buildValue(context, method, currentBlock, retAST->value.get());
        insert(std::make_unique<hir::StoreReturnHIR>(nodeValue), currentBlock);
        insert(std::make_unique<hir::MethodReturnHIR>(), currentBlock);
        currentBlock->hasMethodReturn = true;
    } break;

    case ast::ASTType::kList: {
        assert(false);
    } break;

    case ast::ASTType::kDictionary: {
        assert(false);
    } break;
    }

    return nodeValue;
}

hir::NVID BlockBuilder::buildFinalValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const ast::SequenceAST* sequenceAST) {
    for (const auto& ast : sequenceAST->sequence) {
        currentBlock->finalValue = buildValue(context, method, currentBlock, ast.get());
        // If the last statement built was a MethodReturn we can skip compiling the rest of the sequence.
        if (currentBlock->hasMethodReturn) { break; }
    }
    return currentBlock->finalValue;
}

hir::NVID BlockBuilder::buildIf(ThreadContext* context, library::Method method, Block*& currentBlock,
        const ast::IfAST* ifAST) {
    hir::NVID nodeValue;
    // Compute final value of the condition.
    auto condition = buildFinalValue(context, method, currentBlock, ifAST->condition.get());

    // Add branch to the true block if the condition is true.
    auto trueBranchOwning = std::make_unique<hir::BranchIfTrueHIR>(condition);
    auto trueBranch = trueBranchOwning.get();
    insert(std::move(trueBranchOwning), currentBlock);

    // Insert absolute branch to the false block.
    auto falseBranchOwning = std::make_unique<hir::BranchHIR>();
    hir::BranchHIR* falseBranch = falseBranchOwning.get();
    insert(std::move(falseBranchOwning), currentBlock);

    // Preserve the current block and frame for insertion of the new subframes as children.
    Scope* parentScope = currentBlock->scope;
    Block* conditionBlock = currentBlock;

    // Build the true condition block.
    auto trueScopeOwning = buildInlineBlock(context, method, conditionBlock, ifAST->trueBlock.get());
    Scope* trueScope = trueScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(trueScopeOwning));
    trueBranch->blockId = trueScope->blocks.front()->id;
    conditionBlock->successors.emplace_back(trueScope->blocks.front().get());

    // Build the false condition block.
    auto falseScopeOwning = buildInlineBlock(context, method, conditionBlock, ifAST->falseBlock.get());
    Scope* falseScope = falseScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(falseScopeOwning));
    falseBranch->blockId = falseScope->blocks.front()->id;
    conditionBlock->successors.emplace_back(falseScope->blocks.front().get());

    // Create a new block in the parent frame for code after the if statement.
    auto continueBlock = std::make_unique<Block>(parentScope, parentScope->frame->numberOfBlocks);
    ++parentScope->frame->numberOfBlocks;
    currentBlock = continueBlock.get();
    parentScope->blocks.emplace_back(std::move(continueBlock));

    // Wire trueScope exit block to the continue block here.
    auto exitTrueBranch = std::make_unique<hir::BranchHIR>();
    exitTrueBranch->blockId = currentBlock->id;
    insert(std::move(exitTrueBranch), trueScope->blocks.back().get());
    trueScope->blocks.back()->successors.emplace_back(currentBlock);
    currentBlock->predecessors.emplace_back(trueScope->blocks.back().get());

    // Wire falseFrame exit block to the continue block here.
    auto exitFalseBranch = std::make_unique<hir::BranchHIR>();
    exitFalseBranch->blockId = currentBlock->id;
    insert(std::move(exitFalseBranch), falseScope->blocks.back().get());
    falseScope->blocks.back()->successors.emplace_back(currentBlock);
    currentBlock->predecessors.emplace_back(falseScope->blocks.back().get());

    // Add a Phi with the final value of both the false and true blocks here, and which serves as the value of the
    // if statement overall.
    auto valuePhi = std::make_unique<hir::PhiHIR>();
    valuePhi->addInput(currentBlock->frame->values[trueScope->blocks.back()->finalValue]);
    valuePhi->addInput(currentBlock->frame->values[falseScope->blocks.back()->finalValue]);
    nodeValue = valuePhi->proposeValue(static_cast<hir::NVID>(currentBlock->frame->values.size()));
    currentBlock->frame->values.emplace_back(valuePhi.get());
    currentBlock->localValues.emplace(std::make_pair(nodeValue, nodeValue));
    currentBlock->phis.emplace_back(std::move(valuePhi));

    return nodeValue;
}

hir::NVID BlockBuilder::insert(std::unique_ptr<hir::HIR> hir, Block* block) {
    // Phis should only be inserted by findScopedName().
    assert(hir->opcode != hir::Opcode::kPhi);

    auto valueNumber = static_cast<hir::NVID>(block->frame->values.size());
    auto value = hir->proposeValue(valueNumber);
    // We don't bump the value serial for invalid values (meaning read-only operations)
    if (value != hir::kInvalidNVID) {
        block->frame->values.emplace_back(hir.get());
        block->localValues.emplace(std::make_pair(value, value));
    }
    block->statements.emplace_back(std::move(hir));
    return value;
}

hir::NVID BlockBuilder::findName(ThreadContext* context, const library::Method method, library::Symbol name,
        Block* block) {

    // If this symbol potentially defines a class name look it up in the class library and provide it as a constant.
    if (name.isClassName(context)) {
        auto classDef = context->classLibrary->findClassNamed(name);
        assert(!classDef.isNil());
        auto constant = std::make_unique<hir::ConstantHIR>(classDef.slot(), name);
        constant->value.knownClassName = name;
        hir::NVID nodeValue = insert(std::move(constant), block);
        block->revisions.emplace(std::make_pair(name, nodeValue));
        return nodeValue;
    }

    // Search through local variables for a name.
    std::unordered_map<int32_t, hir::NVID> blockValues;
    std::unordered_set<const Scope*> containingScopes;
    const Scope* scope = block->scope;
    while (scope) {
        containingScopes.emplace(scope);
        scope = scope->parent;
    }
    hir::NVID nodeValue = findScopedName(context, name, block, blockValues, containingScopes);
    if (nodeValue != hir::kInvalidNVID) { return nodeValue; }

    // Local variable search failed, try arguments. How to handle nested frames and argument conflicts?

    // Instance variable search is next.

    // Class variables are last.

    return hir::kInvalidNVID;
}

hir::NVID BlockBuilder::findScopedName(ThreadContext* context, library::Symbol name, Block* block,
        std::unordered_map<Block::ID, hir::NVID>& blockValues,
        const std::unordered_set<const Scope*>& containingScopes) {

    // To avoid any infinite cycles in block loops, return any value found on a previous call on this block.
    auto cacheIter = blockValues.find(block->id);
    if (cacheIter != blockValues.end()) {
        return cacheIter->second;
    }

    // This scope is *shadowing* the variable name if the scope has a variable of the same name and is not within
    // the scope heirarchy of the search, so we should ignore any local revisions.
    bool isShadowed = (block->scope->variableNames.count(name) > 0) && (containingScopes.count(block->scope) == 0);

    auto iter = block->revisions.find(name);
    if (iter != block->revisions.end()) {
        if (!isShadowed) {
            return iter->second;
        }
    }

    // Either no local revision found or the local revision is ignored, we need to search recursively upward. Create
    // a phi for possible insertion into the local map (if not shadowed).
    auto phi = std::make_unique<hir::PhiHIR>();
    auto phiValue = phi->proposeValue(static_cast<hir::NVID>(block->frame->values.size()));
    block->frame->values.emplace_back(phi.get());
    blockValues.emplace(std::make_pair(block->id, phiValue));

    // Search predecessors for the name.
    for (auto pred : block->predecessors) {
        auto foundValue = findScopedName(context, name, pred, blockValues, containingScopes);

        // It is worth thinking through if there are any valid scenarios where one predecessor could return a valid NVID
        // and another predecessor would not. If the shadowing logic is working correctly, it seems that there are not,
        // either all inputs are valid (if possibly trivial) or all inputs are invalid.
        if (foundValue == hir::kInvalidNVID) { return hir::kInvalidNVID; }

        phi->addInput(block->frame->values[foundValue]);
    }

    // We likely had no predecessors, so now have an empty phi, so return invalid ID.
    if (phi->inputs.size() == 0) { return hir::kInvalidNVID; }

    // Phi has inputs but it is trivial?
    auto trivialValue = phi->getTrivialValue();

    // Shadowed variables should always result in trivial phis, due to the fact that Scopes must always have exactly
    // one entry Block with at most one predecessor. The local Scope is shadowing the name we're looking for, and
    // so won't modify the unshadowed value, meaning the value of the name at entry to the Scope should be the same
    // throughout every Block in the scope.
    if (isShadowed) {
        assert(trivialValue != hir::kInvalidNVID);
        // Overwrite block value with the trivial value.
        blockValues.emplace(std::make_pair(block->id, trivialValue));
        return trivialValue;
    }

    hir::NVID finalValue;
    if (trivialValue != hir::kInvalidNVID) {
        finalValue = trivialValue;
        // Mark value as invalid in frame.
        block->frame->values[phiValue] = nullptr;
    } else {
        finalValue = phiValue;
        block->phis.emplace_back(std::move(phi));
    }

    // Update block revisions and the blockValues map with the final values.
    block->revisions.emplace(std::make_pair(name, finalValue));
    block->localValues.emplace(std::make_pair(finalValue, finalValue));
    blockValues.emplace(std::make_pair(block->id, finalValue));

    return finalValue;
}

} // namespace hadron