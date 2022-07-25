#include "hadron/BlockBuilder.hpp"

#include "hadron/ClassLibrary.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Heap.hpp"
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

library::CFGFrame BlockBuilder::buildMethod(ThreadContext* context, const library::BlockAST blockAST,
        const library::Class owningClass) {
    m_owningClass = owningClass;
    return buildFrame(context, blockAST);
}

library::CFGFrame BlockBuilder::buildFrame(ThreadContext* context, const library::BlockAST blockAST) {
    // Build outer frame, root scope, and entry block.
    m_frame = library::CFGFrame::makeCFGFrame(context);
    auto scope = m_frame.rootScope();

    // Add arguments to the prototype frame.
    m_frame.setPrototypeFrame(m_frame.prototypeFrame().addAll(context, blockAST.argumentDefaults()));

    // Include the arguments in the root scope value names set.
    for (int32_t i = 0; i < blockAST.argumentNames().size(); ++i) {
        scope.valueIndices().typedPut(context, blockAST.argumentNames().at(i), m_frame.prototypeFrame().size());
        m_frame.setPrototypeFrame(m_frame.prototypeFrame().add(context, blockAST.argumentDefaults().at(i)));
    }

    scope.setBlocks(scope.blocks().typedAdd(context,
            library::CFGBlock::makeCFGBlock(context, m_frame.numberOfBlocks())));
    m_frame.setNumberOfBlocks(m_frame.numberOfBlocks() + 1);
    m_scopes.push_back(scope);
    m_blocks.push_back(scope.blocks().typedFirst());

    buildFinalValue(context, blockAST.statements());

    // We append a return statement in the final block, if one wasn't already provided.
    if (!m_blocks.back().hasMethodReturn()) {
        m_blocks.back().append(context, m_frame, library::MethodReturnHIR::makeMethodReturnHIR(context).toBase());
    }

    m_blocks.pop_back();
    m_scopes.pop_back();

    return m_frame;
}

library::CFGScope BlockBuilder::buildInlineBlock(ThreadContext* context, library::CFGBlock predecessor,
        const library::BlockAST blockAST) {
    auto scope = library::CFGScope::makeCFGScope(context);
    auto block = library::CFGBlock::makeCFGBlock(context, m_frame.numberOfBlocks());
    block.setPredecessors(block.predecessors().add(context, predecessor.id()));
    m_frame.setNumberOfBlocks(m_frame.numberOfBlocks() + 1);
    scope.setBlocks(scope.blocks().typedAdd(context, block));

    // For now we can't handle inline blocks with arguments (other than "this")
    assert(blockAST.argumentNames().size() <= 1);

    m_scopes.push_back(scope);
    m_blocks.push_back(block);
    buildFinalValue(context, blockAST.statements());
    m_blocks.pop_back();
    m_scopes.pop_back();

    return scope;
}

library::HIRId BlockBuilder::buildValue(ThreadContext* context, const library::AST ast) {
    auto nodeValue = library::HIRId();

    switch(ast.className()) {
    case library::EmptyAST::nameHash():
        nodeValue = m_blocks.back().append(context, m_frame,
                library::ConstantHIR::makeConstantHIR(context, Slot::makeNil()).toBase());
        assert(nodeValue);
        break;

    case library::SequenceAST::nameHash(): {
        auto sequenceAST = library::SequenceAST(ast.slot());
        nodeValue = buildFinalValue(context, sequenceAST);
        assert(nodeValue);
    } break;

    // Blocks encountered here are are block literals, and are candidates for inlining.
    case library::BlockAST::nameHash(): {
        auto blockAST = library::BlockAST(ast.slot());
        auto blockHIR = library::BlockLiteralHIR::makeBlockLiteralHIR(context);
        m_frame.setInnerBlocks(m_frame.innerBlocks().typedAdd(context, blockHIR));
        nodeValue = m_blocks.back().append(context, m_frame, blockHIR.toBase());
        assert(nodeValue);
    } break;

    case library::IfAST::nameHash(): {
        auto ifAST = library::IfAST(ast.slot());
        nodeValue = buildIf(context, ifAST);
        assert(nodeValue);
    } break;

    case library::WhileAST::nameHash(): {
        auto whileAST = library::WhileAST(ast.slot());
        nodeValue = buildWhile(context, whileAST);
        assert(nodeValue);
    } break;

    case library::MessageAST::nameHash(): {
        auto messageAST = library::MessageAST(ast.slot());

        auto messageHIR = library::MessageHIR::makeMessageHIR(context);
        messageHIR.setSelector(messageAST.selector(context));

        // Build arguments.
        for (int32_t i = 0; i < messageAST.arguments().sequence().size(); ++i) {
            auto argAST = library::AST::wrapUnsafe(messageAST.arguments().sequence().at(i));
            messageHIR.addArgument(context, buildValue(context, argAST));
        }

        // Build keyword argument pairs.
        for (int32_t i = 0; i < messageAST.keywordArguments().sequence().size(); ++i) {
            auto argAST = library::AST::wrapUnsafe(messageAST.keywordArguments().sequence().at(i));
            messageHIR.addKeywordArgument(context, buildValue(context, argAST));
        }

        nodeValue = m_blocks.back().append(context, m_frame, messageHIR.toBase());
        assert(nodeValue);
    } break;

    case library::NameAST::nameHash(): {
        auto nameAST = library::NameAST(ast.slot());
        auto findHIR = findName(context, nameAST.name(context), library::HIRId());
        assert(findHIR);
        assert(findHIR.id());
        nodeValue = findHIR.id();
    } break;

    case library::AssignAST::nameHash(): {
        auto assignAST = library::AssignAST(ast.slot());
        nodeValue = buildValue(context, assignAST.value());
        assert(nodeValue);

        auto assign = findName(context, assignAST.name(context), nodeValue);
        assert(assign);
    } break;

    case library::DefineAST::nameHash(): {
        auto defineAST = library::DefineAST(ast.slot());

        // Add the name to variable index tracking.
        m_frame.setVariableNames(m_frame.variableNames().add(context, defineAST.name(context)));
        m_scopes.back().valueIndices().typedPut(context, defineAST.name(context),
                m_frame.prototypeFrame().size());

        // Simple constant definitions don't need to be saved, rather we add the value to initial values array.
        if (defineAST.value().className() == library::ConstantAST::nameHash()) {
            auto constAST = library::ConstantAST(defineAST.value().slot());
            m_frame.setPrototypeFrame(m_frame.prototypeFrame().add(context, constAST.constant()));

            nodeValue = m_blocks.back().append(context, m_frame,
                    library::ConstantHIR::makeConstantHIR(context, constAST.constant()).toBase());
        } else {
            // Complex value, assign nil as the prototype value, then create a write statement here to that offset.
            m_frame.setPrototypeFrame(m_frame.prototypeFrame().add(context, Slot::makeNil()));

            nodeValue = buildValue(context, defineAST.value());
            auto assign = findName(context, defineAST.name(context), nodeValue);
            assert(assign);
        }
        assert(nodeValue);
    } break;

    case library::ConstantAST::nameHash(): {
        auto constAST = library::ConstantAST(ast.slot());
        nodeValue = m_blocks.back().append(context, m_frame, library::ConstantHIR::makeConstantHIR(context,
                constAST.constant()).toBase());
        assert(nodeValue);
    } break;

    case library::MethodReturnAST::nameHash(): {
        auto retAST = library::MethodReturnAST(ast.slot());
        nodeValue = buildValue(context, retAST.value());
        assert(nodeValue);

        m_blocks.back().append(context, m_frame,
                library::StoreReturnHIR::makeStoreReturnHIR(context, nodeValue).toBase());
        m_blocks.back().append(context, m_frame,
                library::MethodReturnHIR::makeMethodReturnHIR(context).toBase());
    } break;

    case library::MultiAssignAST::nameHash(): {
        auto multiAssignAST = library::MultiAssignAST(ast.slot());
        // Final value of multiAssign statements is the array-like type returned by the expression.
        nodeValue = buildValue(context, multiAssignAST.arrayValue());
        assert(nodeValue);

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

            auto indexValue = m_blocks.back().append(context, m_frame,
                    library::ConstantHIR::makeConstantHIR(context, Slot::makeInt32(i)).toBase());
            message.addArgument(context, indexValue);

            auto messageValue = m_blocks.back().append(context, m_frame, message.toBase());
            auto assign = findName(context, nameAST.name(context), messageValue);
            assert(assign);
        }
    } break;
    }

    assert(nodeValue);
    return nodeValue;
}

library::HIRId BlockBuilder::buildFinalValue(ThreadContext* context, const library::SequenceAST sequenceAST) {
    auto finalValue = library::HIRId();

    if (sequenceAST.sequence().size()) {
        for (int32_t i = 0; i < sequenceAST.sequence().size(); ++i) {
            auto ast = library::AST::wrapUnsafe(sequenceAST.sequence().at(i));
            finalValue = buildValue(context, ast);
            m_blocks.back().setFinalValue(finalValue);

            // If the last statement built was a MethodReturn we can skip compiling the rest of the sequence.
            if (m_blocks.back().hasMethodReturn()) { break; }
        }
    } else {
        finalValue = m_blocks.back().append(context, m_frame,
                library::ConstantHIR::makeConstantHIR(context, Slot::makeNil()).toBase());
        m_blocks.back().setFinalValue(finalValue);
    }

    assert(finalValue);
    return finalValue;
}

library::HIRId BlockBuilder::buildIf(ThreadContext* context, const library::IfAST ifAST) {
    // Compute final value of the condition.
    auto condition = buildFinalValue(context, ifAST.condition());

    // Add branch to the true block if the condition is true.
    auto trueBranch = library::BranchIfTrueHIR::makeBranchIfTrueHIR(context, condition);
    m_blocks.back().append(context, m_frame, trueBranch.toBase());

    // Insert absolute branch to the false block.
    auto falseBranch = library::BranchHIR::makeBranchHIR(context);
    m_blocks.back().append(context, m_frame, falseBranch.toBase());

    // Preserve the current block and frame for insertion of the new subframes as children.
    auto parentScope = currentBlock.scope();
    library::CFGBlock conditionBlock = currentBlock;

    // Build the true condition block.
    auto trueScope = buildInlineBlock(context, parentScope, conditionBlock, ifAST.trueBlock());
    parentScope.setSubScopes(parentScope.subScopes().typedAdd(context, trueScope));
    trueBranch.setBlockId(trueScope.blocks().typedFirst().id());
    conditionBlock.setSuccessors(conditionBlock.successors().typedAdd(context, trueScope.blocks().typedFirst()));

    // Build the false condition block.
    auto falseScope = buildInlineBlock(context, parentScope, conditionBlock, ifAST.falseBlock());
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
        auto valuePhi = library::PhiHIR::makePhiHIR(context);
        nodeValue = currentBlock.append(context, valuePhi.toBase());
        valuePhi.addInput(context, currentBlock.frame().values().typedAt(
                trueScope.blocks().typedLast().finalValue().int32()));
        valuePhi.addInput(context, currentBlock.frame().values().typedAt(
                falseScope.blocks().typedLast().finalValue().int32()));
        assert(nodeValue);
    }

    return nodeValue;
}

library::HIRId BlockBuilder::buildWhile(ThreadContext* context, library::CFGBlock& currentBlock,
        const library::WhileAST whileAST) {
    auto parentScope = currentBlock.scope();

    // Build condition block. Note this block is unsealed.
    auto conditionScope = buildInlineBlock(context, parentScope, currentBlock, whileAST.conditionBlock());
    auto conditionEntryBlock = conditionScope.blocks().typedFirst();
    parentScope.setSubScopes(parentScope.subScopes().typedAdd(context, conditionScope));

    // Predecessor block branches to condition block.
    currentBlock.setSuccessors(currentBlock.successors().typedAdd(context, conditionEntryBlock));
    auto predBranch = library::BranchHIR::makeBranchHIR(context);
    predBranch.setBlockId(conditionEntryBlock.id());
    currentBlock.append(context, predBranch.toBase());
    auto conditionExitBlock = conditionScope.blocks().typedLast();

    // Build repeat block.
    auto repeatScope = buildInlineBlock(context, parentScope, conditionExitBlock, whileAST.repeatBlock());
    parentScope.setSubScopes(parentScope.subScopes().typedAdd(context, repeatScope));
    auto repeatExitBlock = repeatScope.blocks().typedLast();

    // Repeat block branches to condition block.
    repeatExitBlock.setSuccessors(repeatExitBlock.successors().typedAdd(context, conditionEntryBlock));
    conditionEntryBlock.setLoopReturnPredIndex(library::Integer(conditionEntryBlock.predecessors().size()));
    conditionEntryBlock.setPredecessors(conditionEntryBlock.predecessors().typedAdd(context, repeatExitBlock));
    auto repeatBranch = library::BranchHIR::makeBranchHIR(context);
    repeatBranch.setBlockId(conditionEntryBlock.id());
    repeatExitBlock.append(context, repeatBranch.toBase());

    // Condition block conditionally jumps to loop block if true.
    conditionExitBlock.setSuccessors(conditionExitBlock.successors().typedAdd(context,
            repeatScope.blocks().typedFirst()));
    auto trueBranch = library::BranchIfTrueHIR::makeBranchIfTrueHIR(context, conditionExitBlock.finalValue());
    trueBranch.setBlockId(repeatScope.blocks().typedFirst().id());
    conditionExitBlock.append(context, trueBranch.toBase());

    // Build continuation block.
    auto continueBlock = library::CFGBlock::makeCFGBlock(context, parentScope, parentScope.frame().numberOfBlocks());
    parentScope.frame().setNumberOfBlocks(parentScope.frame().numberOfBlocks() + 1);
    parentScope.setBlocks(parentScope.blocks().typedAdd(context, continueBlock));

    // Condition block unconditionally jumps to continuation block.
    conditionExitBlock.setSuccessors(conditionExitBlock.successors().typedAdd(context, continueBlock));
    continueBlock.setPredecessors(continueBlock.predecessors().typedAdd(context, conditionExitBlock));
    auto exitBranch = library::BranchHIR::makeBranchHIR(context);
    exitBranch.setBlockId(currentBlock.id());
    conditionExitBlock.append(context, exitBranch.toBase());

    currentBlock = continueBlock;
    // Inlined while loops in LSC always have a value of nil. Since Hadron inlines all while loops, we do the same.
    return currentBlock.append(context, library::ConstantHIR::makeConstantHIR(context, Slot::makeNil()).toBase());
}

library::HIR BlockBuilder::findName(ThreadContext* context, library::Symbol name, library::CFGBlock block,
        library::HIRId toWrite) {
    // If this symbol defines a class name look it up in the class library and provide it as a constant.
    if (name.isClassName(context)) {
        // Class names are read-only.
        assert(!toWrite);
        auto classDef = context->classLibrary->findClassNamed(name);
        if (classDef.isNil()) {
            SPDLOG_CRITICAL("failed to find class named: {}", name.view(context));
            assert(false);
        }
        auto constant = library::ConstantHIR::makeConstantHIR(context, classDef.slot());
        auto id = block.append(context, constant.toBase());
        return block.frame().values().typedAt(id.int32());
    }

    // Search through local values in scope.
    auto searchBlock = block;
    size_t nestedFramesCount = 0;

    do {
        auto scope = searchBlock.scope();

        while (scope) {
            auto index = scope.valueIndices().typedGet(name);
            if (index) {
                // Match found, load any and all needed nested frames.
                auto frameId = library::HIRId();
                for (size_t i = 0; i < nestedFramesCount; ++i) {
                    frameId = block.append(context, library::LoadOuterFrameHIR::makeOuterFrameHIR(context,
                            frameId).toBase());
                }

                if (!toWrite) {
                    auto readFromFrame = library::ReadFromFrameHIR::makeReadFromFrameHIR(context, index.int32(),
                            frameId, name);
                    auto hir = readFromFrame.toBase();
                    block.append(context, hir);
                    return hir;
                }
                auto writeToFrame = library::WriteToFrameHIR::makeWriteToFrameHIR(context, index.int32(), frameId,
                        name, toWrite);
                auto hir = writeToFrame.toBase();
                block.append(context, hir);
                return hir;
            }
            scope = scope.parent();
        }

        if (!searchBlock.frame().outerBlockHIR()) {
            break;
        }

        ++nestedFramesCount;
        searchBlock = searchBlock.frame().outerBlockHIR().owningBlock();
    } while (true);

    // Search instance variables next.
    library::Class classDef = m_owningClass;
    auto instVarIndex = classDef.instVarNames().indexOf(name);
    if (instVarIndex) {
        // Go find the this pointer.
        auto thisHIR = findName(context, context->symbolTable->thisSymbol(), block, library::HIRId());
        assert(thisHIR);
        assert(thisHIR.id());
        if (!toWrite) {
            auto readFromThis = library::ReadFromThisHIR::makeReadFromThisHIR(context, thisHIR.id(),
                    instVarIndex.int32(), name);
            auto hir = readFromThis.toBase();
            block.append(context, hir);
            return hir;
        }
        auto writeToThis = library::WriteToThisHIR::makeWriteToThisHIR(context, thisHIR.id(), instVarIndex.int32(),
                name, toWrite);
        auto hir = writeToThis.toBase();
        block.append(context, hir);
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
        if (classVarOffset) {
            auto classArrayHIR = library::ReadFromContextHIR::makeReadFromContextHIR(context, offsetof(ThreadContext,
                    classVariablesArray), library::Symbol::fromView(context, "_classVariablesArray"));
            auto classArrayId = block.append(context, classArrayHIR.toBase());
            if (!toWrite) {
                auto readFromClass = library::ReadFromClassHIR::makeReadFromClassHIR(context, classArrayId,
                        classVarOffset.int32(), name);
                auto hir = readFromClass.toBase();
                block.append(context, hir);
                return hir;
            }
            auto writeToClass = library::WriteToClassHIR::makeWriteToClassHIR(context, classArrayId,
                    classVarOffset.int32(), name, toWrite);
            auto hir = writeToClass.toBase();
            block.append(context, hir);
            return hir;
        }

        classVarDef = context->classLibrary->findClassNamed(classVarDef.superclass(context));
    }

    // Search constants next.
    library::Class classConstDef = classDef;
    while (!classConstDef.isNil()) {
        auto constIndex = classConstDef.constNames().indexOf(name);
        if (constIndex) {
            // Constants are read-only.
            assert(!toWrite);
            auto constant = library::ConstantHIR::makeConstantHIR(context,
                    classConstDef.constValues().at(constIndex.int32()));
            auto id = block.append(context, constant.toBase());
            assert(id);
            return block.frame().values().typedAt(id.int32());
        }

        classConstDef = context->classLibrary->findClassNamed(classConstDef.superclass(context));
    }

    // Check for special names.
    if (name == context->symbolTable->superSymbol()) {
        auto thisRead = findName(context, context->symbolTable->thisSymbol(), block, library::HIRId());
        // super is read-only.
        assert(!toWrite);
        assert(thisRead);
        assert(thisRead.id());
        auto super = library::RouteToSuperclassHIR::makeRouteToSuperclassHIR(context, thisRead.id());
        auto hir = super.toBase();
        block.append(context, hir);
        return hir;
    }

    if (name == context->symbolTable->thisMethodSymbol()) {
        assert(!toWrite); // thisMethod is read-only.
        auto readFromFrame = library::ReadFromFrameHIR::makeReadFromFrameHIR(context,
                static_cast<int32_t>(offsetof(schema::FramePrivateSchema, method)) / kSlotSize, library::HIRId(),
                context->symbolTable->thisMethodSymbol());
        auto hir = readFromFrame.toBase();
        block.append(context, hir);
        return hir;
    }

    if (name == context->symbolTable->thisProcessSymbol()) {
        assert(!toWrite); // thisProcess is read-only.
        auto readFromContext = library::ReadFromContextHIR::makeReadFromContextHIR(context, offsetof(ThreadContext,
                thisProcess), context->symbolTable->thisProcessSymbol());
        auto hir = readFromContext.toBase();
        block.append(context, hir);
        return hir;
    }

    if (name == context->symbolTable->thisThreadSymbol()) {
        assert(!toWrite); // thisThread is read-only
        auto readFromContext = library::ReadFromContextHIR::makeReadFromContextHIR(context, offsetof(ThreadContext,
                thisThread), context->symbolTable->thisThreadSymbol());
        auto hir = readFromContext.toBase();
        block.append(context, hir);
        return hir;
    }

    SPDLOG_CRITICAL("failed to find name: {}", name.view(context));
    return library::HIR();
}


} // namespace hadron