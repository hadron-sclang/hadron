#include "hadron/BlockBuilder.hpp"

#include "hadron/AST.hpp"
#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Heap.hpp"
#include "hadron/hir/AssignHIR.hpp"
#include "hadron/hir/BlockLiteralHIR.hpp"
#include "hadron/hir/BranchHIR.hpp"
#include "hadron/hir/BranchIfTrueHIR.hpp"
#include "hadron/hir/ConstantHIR.hpp"
#include "hadron/hir/HIR.hpp"
#include "hadron/hir/ImportClassVariableHIR.hpp"
#include "hadron/hir/ImportInstanceVariableHIR.hpp"
#include "hadron/hir/ImportLocalVariableHIR.hpp"
#include "hadron/hir/LoadArgumentHIR.hpp"
#include "hadron/hir/MessageHIR.hpp"
#include "hadron/hir/MethodReturnHIR.hpp"
#include "hadron/hir/PhiHIR.hpp"
#include "hadron/hir/RouteToSuperclassHIR.hpp"
#include "hadron/hir/StoreReturnHIR.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/LinearFrame.hpp"
#include "hadron/Scope.hpp"
#include "hadron/Slot.hpp"
#include "hadron/SymbolTable.hpp"
#include "hadron/ThreadContext.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <cassert>
#include <stack>
#include <string>

namespace hadron {

BlockBuilder::BlockBuilder(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter) { }

std::unique_ptr<Frame> BlockBuilder::buildMethod(ThreadContext* context, const library::Method method,
        const ast::BlockAST* blockAST) {
    return buildFrame(context, method, blockAST, nullptr);
}

std::unique_ptr<Frame> BlockBuilder::buildFrame(ThreadContext* context, const library::Method method,
    const ast::BlockAST* blockAST, hir::BlockLiteralHIR* outerBlockHIR) {
    // Build outer frame, root scope, and entry block.
    auto frame = std::make_unique<Frame>(outerBlockHIR, method, blockAST->argumentNames, blockAST->argumentDefaults);

    // The first block in the root Scope is the "import" block, which we leave empty except for a branch instruction to
    // the next block. This block can be used for insertion of LoadExternalHIR and LoadArgumentHIR instructions in a
    // location that is guaranteed to *dominate* every other block in the graph.
    auto scope = frame->rootScope.get();
    scope->blocks.emplace_back(std::make_unique<Block>(scope, frame->numberOfBlocks));
    ++frame->numberOfBlocks;
    auto block = scope->blocks.front().get();

    // Load all arguments here.
    for (int32_t argIndex = 0; argIndex < frame->argumentOrder.size(); ++argIndex) {
        auto name = frame->argumentOrder.at(argIndex);
        auto loadArg = std::make_unique<hir::LoadArgumentHIR>(argIndex);

        // Set known class and type to *this* pointer.
        if (argIndex == 0) {
            loadArg->typeFlags = TypeFlags::kObjectFlag;
        } else if (blockAST->hasVarArg && argIndex == frame->argumentOrder.size() - 1) {
            // Variable arguments always have Array type.
            loadArg->typeFlags = TypeFlags::kObjectFlag;
        }
        auto argValue = block->append(std::move(loadArg));
        auto nameAssign = std::make_unique<hir::AssignHIR>(name, argValue);
        block->nameAssignments().emplace(std::make_pair(name, nameAssign.get()));
        block->append(std::move(nameAssign));
    }

    // Create a new block for the method body and insert a branch to the new block from the current one.
    scope->blocks.emplace_back(std::make_unique<Block>(scope, frame->numberOfBlocks));
    ++frame->numberOfBlocks;
    auto branch = std::make_unique<hir::BranchHIR>();
    branch->blockId = scope->blocks.back()->id();
    block->append(std::move(branch));

    Block* currentBlock = scope->blocks.back().get();
    block->successors().emplace_back(currentBlock);
    currentBlock->predecessors().emplace_back(block);

    buildFinalValue(context, method, currentBlock, blockAST->statements.get());

    // We append a return statement in the final block, if one wasn't already provided.
    if (!currentBlock->hasMethodReturn()) {
        currentBlock->append(std::make_unique<hir::MethodReturnHIR>());
    }

    return frame;
}

std::unique_ptr<Scope> BlockBuilder::buildInlineBlock(ThreadContext* context, const library::Method method,
        Scope* parentScope, Block* predecessor, const ast::BlockAST* blockAST, bool isSealed) {
    auto scope = std::make_unique<Scope>(parentScope);
    scope->blocks.emplace_back(std::make_unique<Block>(scope.get(), scope->frame->numberOfBlocks, isSealed));
    ++scope->frame->numberOfBlocks;
    auto block = scope->blocks.front().get();
    block->predecessors().emplace_back(predecessor);

    // For now we can't handle inline blocks with arguments
    assert(blockAST->argumentNames.size() <= 1);

    Block* currentBlock = block;
    buildFinalValue(context, method, currentBlock, blockAST->statements.get());

    return scope;
}

hir::ID BlockBuilder::buildValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const ast::AST* ast) {
    hir::ID nodeValue;

    switch(ast->astType) {
    case ast::ASTType::kEmpty:
        nodeValue = currentBlock->append(std::make_unique<hir::ConstantHIR>(Slot::makeNil()));
        break;

    case ast::ASTType::kSequence: {
        const auto seq = reinterpret_cast<const ast::SequenceAST*>(ast);
        nodeValue = buildFinalValue(context, method, currentBlock, seq);
    } break;

    // Blocks encountered here are are block literals, and are candidates for inlining.
    case ast::ASTType::kBlock: {
        const auto blockAST = reinterpret_cast<const ast::BlockAST*>(ast);
        auto blockHIROwning = std::make_unique<hir::BlockLiteralHIR>();
        auto blockHIR = blockHIROwning.get();
        nodeValue = currentBlock->append(std::move(blockHIROwning));
        blockHIR->frame = buildFrame(context, method, blockAST, blockHIR);
    } break;

    case ast::ASTType::kIf: {
        const auto ifAST = reinterpret_cast<const ast::IfAST*>(ast);
        nodeValue = buildIf(context, method, currentBlock, ifAST);
    } break;

    case ast::ASTType::kWhile: {
        const auto whileAST = reinterpret_cast<const ast::WhileAST*>(ast);
        nodeValue = buildWhile(context, method, currentBlock, whileAST);
    } break;

    case ast::ASTType::kMessage: {
        const auto message = reinterpret_cast<const ast::MessageAST*>(ast);

        auto messageHIR = std::make_unique<hir::MessageHIR>();
        messageHIR->selector = message->selector;

        // Build arguments.
        messageHIR->arguments.reserve(message->arguments->sequence.size());
        for (const auto& arg : message->arguments->sequence) {
            messageHIR->addArgument(buildValue(context, method, currentBlock, arg.get()));
        }

        // Build keyword argument pairs.
        messageHIR->keywordArguments.reserve(message->keywordArguments->sequence.size());
        for (const auto& arg : message->keywordArguments->sequence) {
            messageHIR->addKeywordArgument(buildValue(context, method, currentBlock, arg.get()));
        }

        nodeValue = currentBlock->append(std::move(messageHIR));
    } break;

    case ast::ASTType::kName: {
        const auto nameAST = reinterpret_cast<const ast::NameAST*>(ast);
        auto assignHIR = currentBlock->findName(context, nameAST->name);
        assert(assignHIR);
        nodeValue = assignHIR->valueId;
    } break;

    case ast::ASTType::kAssign: {
        const auto assignAST = reinterpret_cast<const ast::AssignAST*>(ast);
        // It's possible that this is an external value, and the code is writing it before reading it. Set up the
        // imports and flow the value here, or detect an error if the name is not found.
        auto assignHIR = currentBlock->findName(context, assignAST->name->name);
        assert(assignHIR);

        nodeValue = buildValue(context, method, currentBlock, assignAST->value.get());
        auto assignOwning = std::make_unique<hir::AssignHIR>(assignAST->name->name, nodeValue);
        assignHIR = assignOwning.get();
        currentBlock->append(std::move(assignOwning));

        // Update the name of the built value to reflect the assignment.
        currentBlock->nameAssignments()[assignAST->name->name] = assignHIR;
    } break;

    case ast::ASTType::kDefine: {
        const auto defineAST = reinterpret_cast<const ast::DefineAST*>(ast);
        currentBlock->scope()->variableNames.emplace(defineAST->name->name);

        nodeValue = buildValue(context, method, currentBlock, defineAST->value.get());
        auto assignOwning = std::make_unique<hir::AssignHIR>(defineAST->name->name, nodeValue);
        auto assignHIR = assignOwning.get();
        currentBlock->append(std::move(assignOwning));
        currentBlock->nameAssignments()[defineAST->name->name] = assignHIR;
    } break;

    case ast::ASTType::kConstant: {
        const auto constAST = reinterpret_cast<const ast::ConstantAST*>(ast);
        nodeValue = currentBlock->append(std::make_unique<hir::ConstantHIR>(constAST->constant));
    } break;

    case ast::ASTType::kMethodReturn: {
        const auto retAST = reinterpret_cast<const ast::MethodReturnAST*>(ast);
        nodeValue = buildValue(context, method, currentBlock, retAST->value.get());
        currentBlock->append(std::make_unique<hir::StoreReturnHIR>(nodeValue));
        currentBlock->append(std::make_unique<hir::MethodReturnHIR>());
    } break;
    }

    return nodeValue;
}

hir::ID BlockBuilder::buildFinalValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const ast::SequenceAST* sequenceAST) {
    for (const auto& ast : sequenceAST->sequence) {
        buildValue(context, method, currentBlock, ast.get());

        // If the last statement built was a MethodReturn we can skip compiling the rest of the sequence.
        if (currentBlock->hasMethodReturn()) { break; }
    }
    return currentBlock->finalValue();
}

hir::ID BlockBuilder::buildIf(ThreadContext* context, library::Method method, Block*& currentBlock,
        const ast::IfAST* ifAST) {
    hir::ID nodeValue;
    // Compute final value of the condition.
    auto condition = buildFinalValue(context, method, currentBlock, ifAST->condition.get());

    // Add branch to the true block if the condition is true.
    auto trueBranchOwning = std::make_unique<hir::BranchIfTrueHIR>(condition);
    auto trueBranch = trueBranchOwning.get();
    currentBlock->append(std::move(trueBranchOwning));

    // Insert absolute branch to the false block.
    auto falseBranchOwning = std::make_unique<hir::BranchHIR>();
    hir::BranchHIR* falseBranch = falseBranchOwning.get();
    currentBlock->append(std::move(falseBranchOwning));

    // Preserve the current block and frame for insertion of the new subframes as children.
    Scope* parentScope = currentBlock->scope();
    Block* conditionBlock = currentBlock;

    // Build the true condition block.
    auto trueScopeOwning = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST->trueBlock.get());
    Scope* trueScope = trueScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(trueScopeOwning));
    trueBranch->blockId = trueScope->blocks.front()->id();
    conditionBlock->successors().emplace_back(trueScope->blocks.front().get());

    // Build the false condition block.
    auto falseScopeOwning = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST->falseBlock.get());
    Scope* falseScope = falseScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(falseScopeOwning));
    falseBranch->blockId = falseScope->blocks.front()->id();
    conditionBlock->successors().emplace_back(falseScope->blocks.front().get());

    // If both conditions return there's no need for a continution block, and building values in this scope is done.
    if (falseScope->blocks.back()->hasMethodReturn() && trueScope->blocks.back()->hasMethodReturn()) {
        conditionBlock->setHasMethodReturn(true);
        currentBlock = conditionBlock;
        return conditionBlock->finalValue();
    }

    // Create a new block in the parent frame for code after the if statement.
    auto continueBlock = std::make_unique<Block>(parentScope, parentScope->frame->numberOfBlocks);
    ++parentScope->frame->numberOfBlocks;
    currentBlock = continueBlock.get();
    parentScope->blocks.emplace_back(std::move(continueBlock));

    // Wire trueScope exit block to the continue block here, if the true block doesn't return.
    if (!trueScope->blocks.back()->hasMethodReturn()) {
        auto exitTrueBranch = std::make_unique<hir::BranchHIR>();
        exitTrueBranch->blockId = currentBlock->id();
        trueScope->blocks.back()->append(std::move(exitTrueBranch));
        trueScope->blocks.back()->successors().emplace_back(currentBlock);
        currentBlock->predecessors().emplace_back(trueScope->blocks.back().get());
        nodeValue = trueScope->blocks.back()->finalValue();
    }

    // Wire falseFrame exit block to the continue block here.
    if (!falseScope->blocks.back()->hasMethodReturn()) {
        auto exitFalseBranch = std::make_unique<hir::BranchHIR>();
        exitFalseBranch->blockId = currentBlock->id();
        falseScope->blocks.back()->append(std::move(exitFalseBranch));
        falseScope->blocks.back()->successors().emplace_back(currentBlock);
        currentBlock->predecessors().emplace_back(falseScope->blocks.back().get());
        nodeValue = falseScope->blocks.back()->finalValue();
    }

    // Add a Phi with the final value of both the false and true blocks here, and which serves as the value of the
    // if statement overall.
    if ((!falseScope->blocks.back()->hasMethodReturn()) && (!trueScope->blocks.back()->hasMethodReturn())) {
        auto valuePhi = std::make_unique<hir::PhiHIR>();
        valuePhi->owningBlock = currentBlock;
        valuePhi->addInput(currentBlock->frame()->values[trueScope->blocks.back()->finalValue()]);
        valuePhi->addInput(currentBlock->frame()->values[falseScope->blocks.back()->finalValue()]);
        nodeValue = valuePhi->proposeValue(static_cast<hir::ID>(currentBlock->frame()->values.size()));
        currentBlock->frame()->values.emplace_back(valuePhi.get());
        currentBlock->phis().emplace_back(std::move(valuePhi));
    }

    return nodeValue;
}

hir::ID BlockBuilder::buildWhile(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const ast::WhileAST* whileAST) {
    Scope* parentScope = currentBlock->scope();

    // Build condition block. Note this block is unsealed.
    auto conditionScopeOwning = buildInlineBlock(context, method, parentScope, currentBlock, whileAST->condition.get(),
            false);
    Scope* conditionScope = conditionScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(conditionScopeOwning));

    // Predecessor block branches to condition block.
    currentBlock->successors().emplace_back(conditionScope->blocks.front().get());
    auto predBranch = std::make_unique<hir::BranchHIR>();
    predBranch->blockId = conditionScope->blocks.front()->id();
    currentBlock->append(std::move(predBranch));
    Block* conditionExitBlock = conditionScope->blocks.back().get();

    // Build repeat block.
    auto repeatScopeOwning = buildInlineBlock(context, method, parentScope, conditionExitBlock,
            whileAST->repeatBlock.get());
    Scope* repeatScope = repeatScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(repeatScopeOwning));
    Block* repeatExitBlock = repeatScope->blocks.back().get();

    // Repeat block branches to condition block.
    repeatExitBlock->successors().emplace_back(conditionScope->blocks.front().get());
    conditionScope->blocks.front()->predecessors().emplace_back(repeatExitBlock);
    auto repeatBranch = std::make_unique<hir::BranchHIR>();
    repeatBranch->blockId = conditionScope->blocks.front()->id();
    repeatExitBlock->append(std::move(repeatBranch));

    // Condition block conditionally jumps to loop block if true.
    conditionExitBlock->successors().emplace_back(repeatScope->blocks.front().get());
    auto trueBranch = std::make_unique<hir::BranchIfTrueHIR>(conditionExitBlock->finalValue());
    trueBranch->blockId = repeatScope->blocks.front()->id();
    conditionExitBlock->append(std::move(trueBranch));

    // Seal the condition block now that the condition predecessors are all in place.
    conditionScope->blocks.front()->seal(context);

    // Build continuation block.
    auto continueBlock = std::make_unique<Block>(parentScope, parentScope->frame->numberOfBlocks);
    ++parentScope->frame->numberOfBlocks;
    currentBlock = continueBlock.get();
    parentScope->blocks.emplace_back(std::move(continueBlock));

    // Condition block unconditionally jumps to continuation block.
    conditionExitBlock->successors().emplace_back(currentBlock);
    currentBlock->predecessors().emplace_back(conditionExitBlock);
    auto exitBranch = std::make_unique<hir::BranchHIR>();
    exitBranch->blockId = currentBlock->id();
    conditionExitBlock->append(std::move(exitBranch));

    // Inlined while loops in LSC always have a value of nil. Since Hadron inlines all while loops, we do the same.
    return currentBlock->append(std::make_unique<hir::ConstantHIR>(Slot::makeNil()));
}

} // namespace hadron