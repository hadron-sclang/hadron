#include "hadron/BlockBuilder.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Heap.hpp"
#include "hadron/LinearFrame.hpp"
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

library::Frame BlockBuilder::buildMethod(ThreadContext* context, const library::Method method,
        const library::BlockAST blockAST) {
    return buildFrame(context, method, blockAST, library::BlockLiteralHIR());
}

library::Frame BlockBuilder::buildFrame(ThreadContext* context, const library::Method method,
    const library::BlockAST blockAST, library::BlockLiteralHIR outerBlockHIR) {
    // Build outer frame, root scope, and entry block.
    auto frame = library::Frame::makeFrame(context, outerBlockHIR, method);
    auto scope = frame.rootScope();

    // Add arguments to the prototype frame.
    frame->prototypeFrame = frame->prototypeFrame.addAll(context, blockAST.argumentDefaults());

    // Include the arguments in the root scope value names set.
    for (int32_t i = 0; i < blockAST.argumentNames().size(); ++i) {
        scope->valueIndices.emplace(std::make_pair(blockAST.argumentNames().at(i), frame->prototypeFrame.size()));
        frame->prototypeFrame = frame->prototypeFrame.add(context, blockAST.argumentDefaults().at(i));
    }

    scope->blocks.emplace_back(std::make_unique<Block>(scope, frame->numberOfBlocks));
    ++frame->numberOfBlocks;
    auto currentBlock = scope->blocks.front().get();

    buildFinalValue(context, method, currentBlock, blockAST.statements());

    // We append a return statement in the final block, if one wasn't already provided.
    if (!currentBlock->hasMethodReturn()) {
        currentBlock->append(std::make_unique<hir::MethodReturnHIR>());
    }

    return frame;
}

std::unique_ptr<Scope> BlockBuilder::buildInlineBlock(ThreadContext* context, const library::Method method,
        Scope* parentScope, Block* predecessor, const library::BlockAST blockAST) {
    auto scope = std::make_unique<Scope>(parentScope);
    scope->blocks.emplace_back(std::make_unique<Block>(scope.get(), scope->frame->numberOfBlocks));
    ++scope->frame->numberOfBlocks;
    auto block = scope->blocks.front().get();
    block->predecessors().emplace_back(predecessor);

    // For now we can't handle inline blocks with arguments (other than "this")
    assert(blockAST.argumentNames().size() <= 1);

    Block* currentBlock = block;
    buildFinalValue(context, method, currentBlock, blockAST.statements());

    return scope;
}

hir::ID BlockBuilder::buildValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const library::AST ast) {
    hir::ID nodeValue = hir::kInvalidID;

    switch(ast.className()) {
    case library::EmptyAST::nameHash():
        nodeValue = currentBlock->append(std::make_unique<hir::ConstantHIR>(Slot::makeNil()));
        assert(nodeValue != hir::kInvalidID);
        break;

    case library::SequenceAST::nameHash(): {
        auto sequenceAST = library::SequenceAST(ast.slot());
        nodeValue = buildFinalValue(context, method, currentBlock, sequenceAST);
        assert(nodeValue != hir::kInvalidID);
    } break;

    // Blocks encountered here are are block literals, and are candidates for inlining.
    case library::BlockAST::nameHash(): {
        auto blockAST = library::BlockAST(ast.slot());
        auto blockHIR = std::make_unique<hir::BlockLiteralHIR>();
        auto& blockRef = currentBlock->frame()->innerBlocks.emplace_back(blockHIR.get());
        nodeValue = currentBlock->append(std::move(blockHIR));
        blockRef->frame = buildFrame(context, method, blockAST, blockRef);
        assert(nodeValue != hir::kInvalidID);
    } break;

    case library::IfAST::nameHash(): {
        auto ifAST = library::IfAST(ast.slot());
        nodeValue = buildIf(context, method, currentBlock, ifAST);
        assert(nodeValue != hir::kInvalidID);
    } break;

    case library::WhileAST::nameHash(): {
        auto whileAST = library::WhileAST(ast.slot());
        nodeValue = buildWhile(context, method, currentBlock, whileAST);
        assert(nodeValue != hir::kInvalidID);
    } break;

    case library::MessageAST::nameHash(): {
        auto messageAST = library::MessageAST(ast.slot());

        auto messageHIR = std::make_unique<hir::MessageHIR>();
        messageHIR->selector = messageAST.selector(context);

        // Build arguments.
        messageHIR->arguments.reserve(messageAST.arguments().sequence().size());
        for (int32_t i = 0; i < messageAST.arguments().sequence().size(); ++i) {
            auto argAST = library::AST::wrapUnsafe(messageAST.arguments().sequence().at(i));
            messageHIR->addArgument(buildValue(context, method, currentBlock, argAST));
        }

        // Build keyword argument pairs.
        messageHIR->keywordArguments.reserve(messageAST.keywordArguments().sequence().size());
        for (int32_t i = 0; i < messageAST.keywordArguments().sequence().size(); ++i) {
            auto argAST = library::AST::wrapUnsafe(messageAST.keywordArguments().sequence().at(i));
            messageHIR->addKeywordArgument(buildValue(context, method, currentBlock, argAST));
        }

        nodeValue = currentBlock->append(std::move(messageHIR));
        assert(nodeValue != hir::kInvalidID);
    } break;

    case library::NameAST::nameHash(): {
        auto nameAST = library::NameAST(ast.slot());
        auto findHIR = findName(context, nameAST.name(context), currentBlock, hir::kInvalidID);
        assert(findHIR);
        assert(findHIR->id != hir::kInvalidID);
        nodeValue = findHIR->id;
    } break;

    case library::AssignAST::nameHash(): {
        auto assignAST = library::AssignAST(ast.slot());
        nodeValue = buildValue(context, method, currentBlock, assignAST.value());
        assert(nodeValue != hir::kInvalidID);

        auto assign = findName(context, assignAST.name(context), currentBlock, nodeValue);
        assert(assign);
    } break;

    case library::DefineAST::nameHash(): {
        auto defineAST = library::DefineAST(ast.slot());

        // Add the name to variable index tracking.
        currentBlock->frame()->variableNames = currentBlock->frame()->variableNames.add(context,
                defineAST.name(context));
        currentBlock->scope()->valueIndices.emplace(std::make_pair(defineAST.name(context),
                currentBlock->frame()->prototypeFrame.size()));

        // Simple constant definitions don't need to be saved, rather we add the value to initial values array.
        if (defineAST.value().className() == library::ConstantAST::nameHash()) {
            auto constAST = library::ConstantAST(defineAST.value().slot());
            currentBlock->frame()->prototypeFrame = currentBlock->frame()->prototypeFrame.add(context,
                    constAST.constant());

            nodeValue = currentBlock->append(std::make_unique<hir::ConstantHIR>(constAST.constant()));
        } else {
            // Complex value, assign nil as the prototype value, then create a write statement here to that offset.
            currentBlock->frame()->prototypeFrame = currentBlock->frame()->prototypeFrame.add(context, Slot::makeNil());

            nodeValue = buildValue(context, method, currentBlock, defineAST.value());
            auto assign = findName(context, defineAST.name(context), currentBlock, nodeValue);
            assert(assign);
        }
    } break;

    case library::ConstantAST::nameHash(): {
        auto constAST = library::ConstantAST(ast.slot());
        nodeValue = currentBlock->append(std::make_unique<hir::ConstantHIR>(constAST.constant()));
        assert(nodeValue != hir::kInvalidID);
    } break;

    case library::MethodReturnAST::nameHash(): {
        auto retAST = library::MethodReturnAST(ast.slot());
        nodeValue = buildValue(context, method, currentBlock, retAST.value());
        assert(nodeValue != hir::kInvalidID);

        currentBlock->append(std::make_unique<hir::StoreReturnHIR>(nodeValue));
        currentBlock->append(std::make_unique<hir::MethodReturnHIR>());
    } break;

    case library::MultiAssignAST::nameHash(): {
        auto multiAssignAST = library::MultiAssignAST(ast.slot());
        // Final value of multiAssign statements is the array-like type returned by the expression.
        nodeValue = buildValue(context, method, currentBlock, multiAssignAST.arrayValue());
        assert(nodeValue != hir::kInvalidID);

        // Each assignment is now translated into a name = nodeValue.at(i) message call.
        auto sequence = multiAssignAST.targetNames().sequence();
        for (int32_t i = 0; i < sequence.size(); ++i) {
            auto nameAST = library::NameAST(sequence.at(i));
            auto message = std::make_unique<hir::MessageHIR>();

            if (multiAssignAST.lastIsRemain() && (i == (sequence.size() - 1))) {
                message->selector = context->symbolTable->copySeriesSymbol();
            } else {
                message->selector = context->symbolTable->atSymbol();
            }

            message->arguments.emplace_back(nodeValue);

            auto indexValue = currentBlock->append(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(i)));
            message->arguments.emplace_back(indexValue);

            auto messageValue = currentBlock->append(std::move(message));
            auto assign = findName(context, nameAST.name(context), currentBlock, messageValue);
            assert(assign);
        }
    } break;
    }

    assert(nodeValue != hir::kInvalidID);
    return nodeValue;
}

hir::ID BlockBuilder::buildFinalValue(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const library::SequenceAST sequenceAST) {
    auto finalValue = hir::kInvalidID;

    if (sequenceAST.sequence().size()) {
        for (int32_t i = 0; i < sequenceAST.sequence().size(); ++i) {
            auto ast = library::AST::wrapUnsafe(sequenceAST.sequence().at(i));
            finalValue = buildValue(context, method, currentBlock, ast);
            currentBlock->setFinalValue(finalValue);

            // If the last statement built was a MethodReturn we can skip compiling the rest of the sequence.
            if (currentBlock->hasMethodReturn()) { break; }
        }
    } else {
        finalValue = currentBlock->append(std::make_unique<hir::ConstantHIR>(Slot::makeNil()));
        currentBlock->setFinalValue(finalValue);
    }

    assert(finalValue != hir::kInvalidID);
    return finalValue;
}

hir::ID BlockBuilder::buildIf(ThreadContext* context, library::Method method, Block*& currentBlock,
        const library::IfAST ifAST) {
    hir::ID nodeValue;
    // Compute final value of the condition.
    auto condition = buildFinalValue(context, method, currentBlock, ifAST.condition());

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
    auto trueScopeOwning = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST.trueBlock());
    Scope* trueScope = trueScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(trueScopeOwning));
    trueBranch->blockId = trueScope->blocks.front()->id();
    conditionBlock->successors().emplace_back(trueScope->blocks.front().get());

    // Build the false condition block.
    auto falseScopeOwning = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST.falseBlock());
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
        assert(nodeValue != hir::kInvalidID);
        assert(currentBlock->frame()->values[nodeValue]);
    }

    // Wire falseFrame exit block to the continue block here.
    if (!falseScope->blocks.back()->hasMethodReturn()) {
        auto exitFalseBranch = std::make_unique<hir::BranchHIR>();
        exitFalseBranch->blockId = currentBlock->id();
        falseScope->blocks.back()->append(std::move(exitFalseBranch));
        falseScope->blocks.back()->successors().emplace_back(currentBlock);
        currentBlock->predecessors().emplace_back(falseScope->blocks.back().get());
        nodeValue = falseScope->blocks.back()->finalValue();
        assert(nodeValue != hir::kInvalidID);
        assert(currentBlock->frame()->values[nodeValue]);
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
        const library::WhileAST whileAST) {
    Scope* parentScope = currentBlock->scope();

    // Build condition block. Note this block is unsealed.
    auto conditionScopeOwning = buildInlineBlock(context, method, parentScope, currentBlock, whileAST.conditionBlock());
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
            whileAST.repeatBlock());
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

hir::HIR* BlockBuilder::findName(ThreadContext* context, library::Symbol name, Block* block, hir::ID toWrite) {
    // If this symbol defines a class name look it up in the class library and provide it as a constant.
    if (name.isClassName(context)) {
        // Class names are read-only.
        if (toWrite != hir::kInvalidID) { return nullptr; }
        auto classDef = context->classLibrary->findClassNamed(name);
        if (classDef.isNil()) {
            SPDLOG_CRITICAL("failed to find class named: {}", name.view(context));
            assert(false);
        }
        auto constant = std::make_unique<hir::ConstantHIR>(classDef.slot());
        auto id = block->append(std::move(constant));
        return block->frame()->values[id];
    }

    // Search through local values in scope.
    Block* searchBlock = block;
    size_t nestedFramesCount = 0;

    do {
        Scope* scope = searchBlock->scope();

        while (scope != nullptr) {
            auto iter = scope->valueIndices.find(name);
            if (iter != scope->valueIndices.end()) {
                // Match found, load any and all needed nested frames.
                auto frameId = hir::kInvalidID;
                for (size_t i = 0; i < nestedFramesCount; ++i) {
                    frameId = block->append(std::make_unique<hir::LoadOuterFrameHIR>(frameId));
                }

                if (toWrite == hir::kInvalidID) {
                    auto readFromFrame = std::make_unique<hir::ReadFromFrameHIR>(iter->second, frameId, name);
                    auto hir = readFromFrame.get();
                    block->append(std::move(readFromFrame));
                    return hir;
                }
                auto writeToFrame = std::make_unique<hir::WriteToFrameHIR>(iter->second, frameId, name, toWrite);
                auto hir = writeToFrame.get();
                block->append(std::move(writeToFrame));
                return hir;
            }
            scope = scope->parent;
        }

        if (!searchBlock->frame()->outerBlockHIR) {
            break;
        }

        ++nestedFramesCount;
        searchBlock = searchBlock->frame()->outerBlockHIR->owningBlock;
    } while (true);

    // Search instance variables next.
    library::Class classDef = block->frame()->method.ownerClass();
    auto instVarIndex = classDef.instVarNames().indexOf(name);
    if (instVarIndex.isInt32()) {
        // Go find the this pointer.
        auto thisHir = findName(context, context->symbolTable->thisSymbol(), block, hir::kInvalidID);
        assert(thisHir);
        assert(thisHir->id != hir::kInvalidID);
        if (toWrite == hir::kInvalidID) {
            auto readFromThis = std::make_unique<hir::ReadFromThisHIR>(thisHir->id, instVarIndex.getInt32(), name);
            auto hir = readFromThis.get();
            block->append(std::move(readFromThis));
            return hir;
        }
        auto writeToThis = std::make_unique<hir::WriteToThisHIR>(thisHir->id, instVarIndex.getInt32(), name,
            toWrite);
        auto hir = writeToThis.get();
        block->append(std::move(writeToThis));
        return hir;
    }

    auto className = classDef.name(context).view(context);
    bool isMetaClass = (className.size() > 5) && (className.substr(0, 5) == "Meta_");

    // Meta_ classes to have access to the class variables and constants of their associated class, so adjust the
    // classDef to point to the associated class instead.
    if (isMetaClass) {
        className = className.substr(5);
        classDef = context->classLibrary->findClassNamed(library::Symbol::fromView(context, className));
        assert(!classDef.isNil());
    }

    // Search class variables next, starting from this class and up through all parents.
    library::Class classVarDef = classDef;
    while (!classVarDef.isNil()) {
        auto classVarOffset = classVarDef.classVarNames().indexOf(name);
        if (classVarOffset.isInt32()) {
            // TODO: probably better to load this from ThreadContext at runtime.
            auto classArray = block->append(std::make_unique<hir::ConstantHIR>(
                    context->classLibrary->classVariables().slot()));
            if (toWrite == hir::kInvalidID) {
                auto readFromClass = std::make_unique<hir::ReadFromClassHIR>(classArray, classVarOffset.getInt32(),
                        name);
                auto hir = readFromClass.get();
                block->append(std::move(readFromClass));
                return hir;
            }
            auto writeToClass = std::make_unique<hir::WriteToClassHIR>(classArray, classVarOffset.getInt32(),
                    name, toWrite);
            auto hir = writeToClass.get();
            block->append(std::move(writeToClass));
            return hir;
        }

        classVarDef = context->classLibrary->findClassNamed(classVarDef.superclass(context));
    }

    // Search constants next.
    library::Class classConstDef = classDef;
    while (!classConstDef.isNil()) {
        auto constIndex = classConstDef.constNames().indexOf(name);
        if (constIndex.isInt32()) {
            // Constants are read-only.
            if (toWrite != hir::kInvalidID) { return nullptr; }
            auto constant = std::make_unique<hir::ConstantHIR>(classConstDef.constValues().at(constIndex.getInt32()));
            auto id = block->append(std::move(constant));
            assert(id != hir::kInvalidID);
            return block->frame()->values[id];
        }

        classConstDef = context->classLibrary->findClassNamed(classConstDef.superclass(context));
    }

    // Check for special names.
    if (name == context->symbolTable->superSymbol()) {
        auto thisRead = findName(context, context->symbolTable->thisSymbol(), block, hir::kInvalidID);
        assert(thisRead);
        assert(thisRead->id != hir::kInvalidID);
        auto super = std::make_unique<hir::RouteToSuperclassHIR>(thisRead->id);
        auto hir = super.get();
        block->append(std::move(super));
        return hir;
    }

    if (name == context->symbolTable->thisMethodSymbol()) {
        auto readFromFrame = std::make_unique<hir::ReadFromFrameHIR>(
                static_cast<int32_t>(offsetof(schema::FramePrivateSchema, method)) / kSlotSize,
                hir::kInvalidID,
                context->symbolTable->thisMethodSymbol());
        auto hir = readFromFrame.get();
        block->append(std::move(readFromFrame));
        return hir;
    }

    if (name == context->symbolTable->thisProcessSymbol()) {
        auto readFromContext = std::make_unique<hir::ReadFromContextHIR>(offsetof(ThreadContext, thisProcess),
            context->symbolTable->thisProcessSymbol());
        auto hir = readFromContext.get();
        block->append(std::move(readFromContext));
        return hir;
    }

    if (name == context->symbolTable->thisThreadSymbol()) {
        auto readFromContext = std::make_unique<hir::ReadFromContextHIR>(offsetof(ThreadContext, thisThread),
            context->symbolTable->thisThreadSymbol());
        auto hir = readFromContext.get();
        block->append(std::move(readFromContext));
        return hir;
    }

    SPDLOG_CRITICAL("failed to find name: {}", name.view(context));
    return nullptr;
}


} // namespace hadron