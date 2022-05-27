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

library::CFGFrame BlockBuilder::buildMethod(ThreadContext* context, const library::Method method,
        const library::BlockAST blockAST) {
    return buildFrame(context, method, blockAST, library::BlockLiteralHIR());
}

library::CFGFrame BlockBuilder::buildFrame(ThreadContext* context, const library::Method method,
    const library::BlockAST blockAST, library::BlockLiteralHIR outerBlockHIR) {
    // Build outer frame, root scope, and entry block.
    auto frame = library::CFGFrame::makeCFGFrame(context, outerBlockHIR, method);
    auto scope = frame.rootScope();

    // Add arguments to the prototype frame.
    frame.setPrototypeFrame(frame.prototypeFrame().addAll(context, blockAST.argumentDefaults()));

    // Include the arguments in the root scope value names set.
    for (int32_t i = 0; i < blockAST.argumentNames().size(); ++i) {
        scope.valueIndices().typedPut(context, blockAST.argumentNames().at(i), frame.prototypeFrame().size());
        frame.setPrototypeFrame(frame.prototypeFrame().add(context, blockAST.argumentDefaults().at(i)));
    }

    scope.setBlocks(scope.blocks().typedAdd(context,
            library::CFGBlock::makeCFGBlock(context, scope, frame.numberOfBlocks())));
    frame.setNumberOfBlocks(frame.numberOfBlocks() + 1);
    auto currentBlock = scope.blocks().typedFirst();

    buildFinalValue(context, method, currentBlock, blockAST.statements());

    // We append a return statement in the final block, if one wasn't already provided.
    if (!currentBlock.hasMethodReturn()) {
        currentBlock.append(context, library::MethodReturnHIR::makeMethodReturnHIR(context).toBase());
    }

    return frame;
}

library::CFGScope BlockBuilder::buildInlineBlock(ThreadContext* context, const library::Method method,
        library::CFGScope parentScope, library::CFGBlock predecessor, const library::BlockAST blockAST) {
    auto scope = library::CFGScope::makeSubCFGScope(context, parentScope);
    auto block = library::CFGBlock::makeCFGBlock(context, scope, scope.frame().numberOfBlocks());
    block.setPredecessors(block.predecessors().typedAdd(context, predecessor));
    scope.frame().setNumberOfBlocks(scope.frame().numberOfBlocks() + 1);
    scope.setBlocks(scope.blocks().typedAdd(context, block));

    // For now we can't handle inline blocks with arguments (other than "this")
    assert(blockAST.argumentNames().size() <= 1);

    auto currentBlock = block;
    buildFinalValue(context, method, currentBlock, blockAST.statements());

    return scope;
}

library::HIRId BlockBuilder::buildValue(ThreadContext* context, const library::Method method,
        library::CFGBlock& currentBlock, const library::AST ast) {
    auto nodeValue = library::HIRId();

    switch(ast.className()) {
    case library::EmptyAST::nameHash():
        nodeValue = currentBlock.append(context,
                library::ConstantHIR::makeConstantHIR(context, Slot::makeNil()).toBase());
        assert(nodeValue);
        break;

    case library::SequenceAST::nameHash(): {
        auto sequenceAST = library::SequenceAST(ast.slot());
        nodeValue = buildFinalValue(context, method, currentBlock, sequenceAST);
        assert(nodeValue);
    } break;

    // Blocks encountered here are are block literals, and are candidates for inlining.
    case library::BlockAST::nameHash(): {
        auto blockAST = library::BlockAST(ast.slot());
        auto blockHIR = library::BlockLiteralHIR::makeBlockLiteralHIR(context);
        currentBlock.frame().setInnerBlocks(currentBlock.frame().innerBlocks().typedAdd(context, blockHIR));
        nodeValue = currentBlock.append(context, blockHIR.toBase());
        blockHIR.setFrame(buildFrame(context, method, blockAST, blockHIR));
        assert(nodeValue);
    } break;

    case library::IfAST::nameHash(): {
        auto ifAST = library::IfAST(ast.slot());
        nodeValue = buildIf(context, method, currentBlock, ifAST);
        assert(nodeValue);
    } break;

    case library::WhileAST::nameHash(): {
        auto whileAST = library::WhileAST(ast.slot());
        nodeValue = buildWhile(context, method, currentBlock, whileAST);
        assert(nodeValue);
    } break;

    case library::MessageAST::nameHash(): {
        auto messageAST = library::MessageAST(ast.slot());

        auto messageHIR = library::MessageHIR::makeMessageHIR(context);
        messageHIR.setSelector(messageAST.selector(context));

        // Build arguments.
        for (int32_t i = 0; i < messageAST.arguments().sequence().size(); ++i) {
            auto argAST = library::AST::wrapUnsafe(messageAST.arguments().sequence().at(i));
            messageHIR.addArgument(context, buildValue(context, method, currentBlock, argAST));
        }

        // Build keyword argument pairs.
        for (int32_t i = 0; i < messageAST.keywordArguments().sequence().size(); ++i) {
            auto argAST = library::AST::wrapUnsafe(messageAST.keywordArguments().sequence().at(i));
            messageHIR.addKeywordArgument(context, buildValue(context, method, currentBlock, argAST));
        }

        nodeValue = currentBlock.append(context, messageHIR.toBase());
        assert(nodeValue);
    } break;

    case library::NameAST::nameHash(): {
        auto nameAST = library::NameAST(ast.slot());
        auto findHIR = findName(context, nameAST.name(context), currentBlock, library::HIRId());
        assert(findHIR);
        assert(findHIR.id());
        nodeValue = findHIR.id();
    } break;

    case library::AssignAST::nameHash(): {
        auto assignAST = library::AssignAST(ast.slot());
        nodeValue = buildValue(context, method, currentBlock, assignAST.value());
        assert(nodeValue);

        auto assign = findName(context, assignAST.name(context), currentBlock, nodeValue);
        assert(assign);
    } break;

    case library::DefineAST::nameHash(): {
        auto defineAST = library::DefineAST(ast.slot());

        // Add the name to variable index tracking.
        currentBlock.frame().setVariableNames(currentBlock.frame().variableNames().add(context,
                defineAST.name(context)));
        currentBlock.scope().valueIndices().typedPut(context, defineAST.name(context),
                currentBlock.frame().prototypeFrame().size());

        // Simple constant definitions don't need to be saved, rather we add the value to initial values array.
        if (defineAST.value().className() == library::ConstantAST::nameHash()) {
            auto constAST = library::ConstantAST(defineAST.value().slot());
            currentBlock.frame().setPrototypeFrame(currentBlock.frame().prototypeFrame().add(context,
                    constAST.constant()));

            nodeValue = currentBlock.append(context,
                    library::ConstantHIR::makeConstantHIR(context, constAST.constant()).toBase());
        } else {
            // Complex value, assign nil as the prototype value, then create a write statement here to that offset.
            currentBlock.frame().setPrototypeFrame(currentBlock.frame().prototypeFrame().add(context, Slot::makeNil()));

            nodeValue = buildValue(context, method, currentBlock, defineAST.value());
            auto assign = findName(context, defineAST.name(context), currentBlock, nodeValue);
            assert(assign);
        }
        assert(nodeValue);
    } break;

    case library::ConstantAST::nameHash(): {
        auto constAST = library::ConstantAST(ast.slot());
        nodeValue = currentBlock.append(context, library::ConstantHIR::makeConstantHIR(context,
                constAST.constant()).toBase());
        assert(nodeValue);
    } break;

    case library::MethodReturnAST::nameHash(): {
        auto retAST = library::MethodReturnAST(ast.slot());
        nodeValue = buildValue(context, method, currentBlock, retAST.value());
        assert(nodeValue);

        currentBlock.append(context, library::StoreReturnHIR::makeStoreReturnHIR(context, nodeValue).toBase());
        currentBlock.append(context, library::MethodReturnHIR::makeMethodReturnHIR(context).toBase());
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
            auto message = library::MessageHIR::makeMessageHIR(context);

            if (multiAssignAST.lastIsRemain() && (i == (sequence.size() - 1))) {
                message.setSelector(context->symbolTable->copySeriesSymbol());
            } else {
                message.setSelector(context->symbolTable->atSymbol());
            }

            message.addArgument(context, nodeValue);

            auto indexValue = currentBlock.append(context,
                    library::ConstantHIR::makeConstantHIR(context, Slot::makeInt32(i)).toBase());
            message.addArgument(context, indexValue);

            auto messageValue = currentBlock.append(context, message.toBase());
            auto assign = findName(context, nameAST.name(context), currentBlock, messageValue);
            assert(assign);
        }
    } break;
    }

    assert(nodeValue);
    return nodeValue;
}

library::HIRId BlockBuilder::buildFinalValue(ThreadContext* context, const library::Method method,
        library::CFGBlock& currentBlock, const library::SequenceAST sequenceAST) {
    auto finalValue = library::HIRId();

    if (sequenceAST.sequence().size()) {
        for (int32_t i = 0; i < sequenceAST.sequence().size(); ++i) {
            auto ast = library::AST::wrapUnsafe(sequenceAST.sequence().at(i));
            finalValue = buildValue(context, method, currentBlock, ast);
            currentBlock.setFinalValue(finalValue);

            // If the last statement built was a MethodReturn we can skip compiling the rest of the sequence.
            if (currentBlock.hasMethodReturn()) { break; }
        }
    } else {
        finalValue = currentBlock.append(context,
                library::ConstantHIR::makeConstantHIR(context, Slot::makeNil()).toBase());
        currentBlock.setFinalValue(finalValue);
    }

    assert(finalValue);
    return finalValue;
}

library::HIRId BlockBuilder::buildIf(ThreadContext* context, library::Method method, library::CFGBlock& currentBlock,
        const library::IfAST ifAST) {
    // Compute final value of the condition.
    auto condition = buildFinalValue(context, method, currentBlock, ifAST.condition());

    // Add branch to the true block if the condition is true.
    auto trueBranch = library::BranchIfTrueHIR::makeBranchIfTrueHIR(context, condition);
    currentBlock.append(context, trueBranch.toBase());

    // Insert absolute branch to the false block.
    auto falseBranch = library::BranchHIR::makeBranchHIR(context);
    currentBlock.append(context, falseBranch.toBase());

    // Preserve the current block and frame for insertion of the new subframes as children.
    auto parentScope = currentBlock.scope();
    library::CFGBlock conditionBlock = currentBlock;

    // Build the true condition block.
    auto trueScope = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST.trueBlock());
    parentScope.setSubScopes(parentScope.subScopes().typedAdd(context, trueScope));
    trueBranch.setBlockId(trueScope.blocks().typedFirst().id());
    conditionBlock.setSuccessors(conditionBlock.successors().typedAdd(context, trueScope.blocks().typedFirst()));

    // Build the false condition block.
    auto falseScope = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST.falseBlock());
    parentScope.setSubScopes(parentScope.subScopes().typedAdd(context, falseScope));
    falseBranch.setBlockId(falseScope.blocks().typedFirst().id());
    conditionBlock.setSuccessors(conditionBlock.successors().typedAdd(context, falseScope.blocks().typedFirst()));

    // If both conditions return there's no need for a continution block, and building values in this scope is done.
    if (falseScope.blocks().typedLast().hasMethodReturn() && trueScope.blocks().typedLast().hasMethodReturn()) {
        conditionBlock.setHasMethodReturn(true);
        currentBlock = conditionBlock;
        return conditionBlock.finalValue();
    }

    // Create a new block in the parent frame for code after the if statement.
    auto continueBlock = library::CFGBlock::makeCFGBlock(context, parentScope, parentScope.frame().numberOfBlocks());
    parentScope.frame().setNumberOfBlocks(parentScope.frame().numberOfBlocks() + 1);
    currentBlock = continueBlock;
    parentScope.setBlocks(parentScope.blocks().typedAdd(context, continueBlock));

    library::HIRId nodeValue = library::HIRId();

    // Wire trueScope exit block to the continue block here, if the true block doesn't return.
    if (!trueScope.blocks().typedLast().hasMethodReturn()) {
        auto exitTrueBranch = library::BranchHIR::makeBranchHIR(context);
        exitTrueBranch.setBlockId(currentBlock.id());
        auto lastBlock = trueScope.blocks().typedLast();
        lastBlock.append(context, exitTrueBranch.toBase());
        lastBlock.setSuccessors(lastBlock.successors().typedAdd(context, currentBlock));
        currentBlock.setPredecessors(currentBlock.predecessors().typedAdd(context, lastBlock));
        nodeValue = lastBlock.finalValue();
        assert(nodeValue);
    }

    // Wire falseFrame exit block to the continue block here.
    if (!falseScope.blocks().typedLast().hasMethodReturn()) {
        auto exitFalseBranch = library::BranchHIR::makeBranchHIR(context);
        exitFalseBranch.setBlockId(currentBlock.id());
        auto lastBlock = falseScope.blocks().typedLast();
        lastBlock.append(context, exitFalseBranch.toBase());
        lastBlock.setSuccessors(lastBlock.successors().typedAdd(context, currentBlock));
        currentBlock.setPredecessors(currentBlock.predecessors().typedAdd(context, lastBlock));
        nodeValue = lastBlock.finalValue();
        assert(nodeValue);
    }

    // Add a Phi with the final value of both the false and true blocks here, and which serves as the value of the
    // if statement overall.
    if ((!falseScope.blocks().typedLast().hasMethodReturn()) && (!trueScope.blocks().typedLast().hasMethodReturn())) {
        auto valuePhi = // std::make_unique<hir::PhiHIR>();
        valuePhi->owningBlock = currentBlock;
        valuePhi->addInput(currentBlock->frame()->values[trueScope->blocks.back()->finalValue()]);
        valuePhi->addInput(currentBlock->frame()->values[falseScope->blocks.back()->finalValue()]);
        nodeValue = valuePhi->proposeValue(static_cast<hir::ID>(currentBlock->frame()->values.size()));
        currentBlock->frame()->values.emplace_back(valuePhi.get());
        currentBlock->phis().emplace_back(std::move(valuePhi));
        assert(nodeValue);
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