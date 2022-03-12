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
#include <string>

namespace hadron {

BlockBuilder::BlockBuilder(std::shared_ptr<ErrorReporter> errorReporter):
    m_errorReporter(errorReporter) { }

BlockBuilder::~BlockBuilder() { }

std::unique_ptr<Frame> BlockBuilder::buildMethod(ThreadContext* context, const library::Method method,
        const ast::BlockAST* blockAST) {
    return buildFrame(context, method, blockAST, nullptr);
}

std::unique_ptr<Frame> BlockBuilder::buildFrame(ThreadContext* context, const library::Method method,
    const ast::BlockAST* blockAST, Block* outerBlock) {
    // Build outer frame, root scope, and entry block.
    auto frame = std::make_unique<Frame>();
    frame->outerBlock = outerBlock;
    frame->argumentOrder = blockAST->argumentNames;
    frame->argumentDefaults = blockAST->argumentDefaults;

    // The first block in the root Scope is the "import" block, which we leave empty except for a branch instruction to
    // the next block. This block can be used for insertion of LoadExternalHIR and LoadArgumentHIR instructions in a
    // location that is guaranteed to *dominate* every other block in the graph.
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

        block->revisions.emplace(std::make_pair(name, append(std::move(loadArg), block)));
    }

    // Create a new block for the method body and insert a branch to the new block from the current one.
    scope->blocks.emplace_back(std::make_unique<Block>(scope, frame->numberOfBlocks));
    ++frame->numberOfBlocks;
    auto branch = std::make_unique<hir::BranchHIR>();
    branch->blockId = scope->blocks.back()->id;
    append(std::move(branch), block);

    Block* currentBlock = scope->blocks.back().get();
    block->successors.emplace_back(currentBlock);
    currentBlock->predecessors.emplace_back(block);

    currentBlock->finalValue = buildFinalValue(context, method, currentBlock, blockAST->statements.get());

     // We append a return statement in the final block, if one wasn't already provided.
    if (!currentBlock->hasMethodReturn) {
        append(std::make_unique<hir::MethodReturnHIR>(), currentBlock);
        currentBlock->hasMethodReturn = true;
    }

    return frame;
}

std::unique_ptr<Scope> BlockBuilder::buildInlineBlock(ThreadContext* context, const library::Method method,
        Scope* parentScope, Block* predecessor, const ast::BlockAST* blockAST, bool isSealed) {
    auto scope = std::make_unique<Scope>(parentScope);
    scope->blocks.emplace_back(std::make_unique<Block>(scope.get(), scope->frame->numberOfBlocks));
    ++scope->frame->numberOfBlocks;
    auto block = scope->blocks.front().get();
    block->predecessors.emplace_back(predecessor);
    block->isSealed = isSealed;

    // Inline blocks can have arguments but they are treated as constants with their default values. We skip the
    // *this* pointer for inline blocks so it doesn't shadow the frame-level this pointer.
    for (int32_t argIndex = 1; argIndex < blockAST->argumentNames.size(); ++argIndex) {
        auto name = blockAST->argumentNames.at(argIndex);
        auto value = blockAST->argumentDefaults.at(argIndex);
        block->revisions.emplace(std::make_pair(name,
                append(std::make_unique<hir::ConstantHIR>(value, name), block)));
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
        nodeValue = append(std::make_unique<hir::ConstantHIR>(Slot::makeNil()), currentBlock);
        break;

    case ast::ASTType::kSequence: {
        const auto seq = reinterpret_cast<const ast::SequenceAST*>(ast);
        nodeValue = buildFinalValue(context, method, currentBlock, seq);
    } break;

    // Blocks encountered here are are block literals, and are candidates for inlining.
    case ast::ASTType::kBlock: {
        const auto blockAST = reinterpret_cast<const ast::BlockAST*>(ast);
        auto blockHIR = std::make_unique<hir::BlockLiteralHIR>();
        blockHIR->frame = buildFrame(context, method, blockAST, currentBlock);
        nodeValue = append(std::move(blockHIR), currentBlock);
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

        nodeValue = append(std::move(messageHIR), currentBlock);
    } break;

    case ast::ASTType::kName: {
        const auto nameAST = reinterpret_cast<const ast::NameAST*>(ast);
        nodeValue = findName(context, method, nameAST->name, currentBlock);
        assert(nodeValue != hir::kInvalidNVID);
    } break;

    case ast::ASTType::kAssign: {
        const auto assign = reinterpret_cast<const ast::AssignAST*>(ast);
        // It's possible that this is an external value, and the code is writing it before reading it. Set up the
        // imports and flow the value here, or detect an error if the name is not found.
        auto nameValue = findName(context, method, assign->name->name, currentBlock);
        assert(nameValue != hir::kInvalidNVID);

        auto assignValue = buildValue(context, method, currentBlock, assign->value.get());
        nodeValue = append(std::make_unique<hir::AssignHIR>(assign->name->name,
                currentBlock->frame->values[assignValue]), currentBlock);

        // Update the name of the built value to reflect the assignment.
        currentBlock->revisions[assign->name->name] = nodeValue;
    } break;

    case ast::ASTType::kDefine: {
        const auto defineAST = reinterpret_cast<const ast::DefineAST*>(ast);
        currentBlock->scope->variableNames.emplace(defineAST->name->name);

        auto assignValue = buildValue(context, method, currentBlock, defineAST->value.get());
        nodeValue = append(std::make_unique<hir::AssignHIR>(defineAST->name->name,
                currentBlock->frame->values[assignValue]), currentBlock);
        currentBlock->revisions[defineAST->name->name] = nodeValue;
    } break;

    case ast::ASTType::kConstant: {
        const auto constAST = reinterpret_cast<const ast::ConstantAST*>(ast);
        nodeValue = append(std::make_unique<hir::ConstantHIR>(constAST->constant), currentBlock);
    } break;

    case ast::ASTType::kMethodReturn: {
        const auto retAST = reinterpret_cast<const ast::MethodReturnAST*>(ast);
        nodeValue = buildValue(context, method, currentBlock, retAST->value.get());
        append(std::make_unique<hir::StoreReturnHIR>(nodeValue), currentBlock);
        append(std::make_unique<hir::MethodReturnHIR>(), currentBlock);
        currentBlock->hasMethodReturn = true;
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
    append(std::move(trueBranchOwning), currentBlock);

    // Insert absolute branch to the false block.
    auto falseBranchOwning = std::make_unique<hir::BranchHIR>();
    hir::BranchHIR* falseBranch = falseBranchOwning.get();
    append(std::move(falseBranchOwning), currentBlock);

    // Preserve the current block and frame for insertion of the new subframes as children.
    Scope* parentScope = currentBlock->scope;
    Block* conditionBlock = currentBlock;

    // Build the true condition block.
    auto trueScopeOwning = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST->trueBlock.get());
    Scope* trueScope = trueScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(trueScopeOwning));
    trueBranch->blockId = trueScope->blocks.front()->id;
    conditionBlock->successors.emplace_back(trueScope->blocks.front().get());

    // Build the false condition block.
    auto falseScopeOwning = buildInlineBlock(context, method, parentScope, conditionBlock, ifAST->falseBlock.get());
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
    append(std::move(exitTrueBranch), trueScope->blocks.back().get());
    trueScope->blocks.back()->successors.emplace_back(currentBlock);
    currentBlock->predecessors.emplace_back(trueScope->blocks.back().get());

    // Wire falseFrame exit block to the continue block here.
    auto exitFalseBranch = std::make_unique<hir::BranchHIR>();
    exitFalseBranch->blockId = currentBlock->id;
    append(std::move(exitFalseBranch), falseScope->blocks.back().get());
    falseScope->blocks.back()->successors.emplace_back(currentBlock);
    currentBlock->predecessors.emplace_back(falseScope->blocks.back().get());

    // Add a Phi with the final value of both the false and true blocks here, and which serves as the value of the
    // if statement overall.
    auto valuePhi = std::make_unique<hir::PhiHIR>();
    valuePhi->owningBlock = currentBlock;
    valuePhi->addInput(currentBlock->frame->values[trueScope->blocks.back()->finalValue]);
    valuePhi->addInput(currentBlock->frame->values[falseScope->blocks.back()->finalValue]);
    nodeValue = valuePhi->proposeValue(static_cast<hir::NVID>(currentBlock->frame->values.size()));
    currentBlock->frame->values.emplace_back(valuePhi.get());
    currentBlock->phis.emplace_back(std::move(valuePhi));

    return nodeValue;
}

hir::NVID BlockBuilder::buildWhile(ThreadContext* context, const library::Method method, Block*& currentBlock,
        const ast::WhileAST* whileAST) {
    Scope* parentScope = currentBlock->scope;

    // Build condition block. Note this block is unsealed.
    auto conditionScopeOwning = buildInlineBlock(context, method, parentScope, currentBlock, whileAST->condition.get(),
            false);
    Scope* conditionScope = conditionScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(conditionScopeOwning));

    // Predecessor block branches to condition block.
    currentBlock->successors.emplace_back(conditionScope->blocks.front().get());
    auto predBranch = std::make_unique<hir::BranchHIR>();
    predBranch->blockId = conditionScope->blocks.front()->id;
    append(std::move(predBranch), currentBlock);
    Block* conditionExitBlock = conditionScope->blocks.back().get();

    // Build repeat block.
    auto repeatScopeOwning = buildInlineBlock(context, method, parentScope, conditionExitBlock,
            whileAST->repeatBlock.get());
    Scope* repeatScope = repeatScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(repeatScopeOwning));
    Block* repeatExitBlock = repeatScope->blocks.back().get();

    // Repeat block branches to condition block.
    repeatExitBlock->successors.emplace_back(conditionScope->blocks.front().get());
    conditionScope->blocks.front()->predecessors.emplace_back(repeatExitBlock);
    auto repeatBranch = std::make_unique<hir::BranchHIR>();
    repeatBranch->blockId = conditionScope->blocks.front()->id;
    append(std::move(repeatBranch), repeatExitBlock);

    // Condition block conditionally jumps to loop block if true.
    conditionExitBlock->successors.emplace_back(repeatScope->blocks.front().get());
    auto trueBranch = std::make_unique<hir::BranchIfTrueHIR>(conditionExitBlock->finalValue);
    trueBranch->blockId = repeatScope->blocks.front()->id;
    append(std::move(trueBranch), conditionExitBlock);

    // Seal the condition block now that the condition predecessors are all in place.
    sealBlock(context, method, conditionScope->blocks.front().get());

    // Build continuation block.
    auto continueBlock = std::make_unique<Block>(parentScope, parentScope->frame->numberOfBlocks);
    ++parentScope->frame->numberOfBlocks;
    currentBlock = continueBlock.get();
    parentScope->blocks.emplace_back(std::move(continueBlock));

    // Condition block unconditionally jumps to continuation block.
    conditionExitBlock->successors.emplace_back(currentBlock);
    currentBlock->predecessors.emplace_back(conditionExitBlock);
    auto exitBranch = std::make_unique<hir::BranchHIR>();
    exitBranch->blockId = currentBlock->id;
    append(std::move(exitBranch), conditionExitBlock);

    // Inlined while loops in LSC always have a value of nil. Since Hadron inlines all while loops, we do the same.
    return append(std::make_unique<hir::ConstantHIR>(Slot::makeNil()), currentBlock);
}

void BlockBuilder::sealBlock(ThreadContext* context, const library::Method method, Block* block) {
    assert(!block->isSealed);
    block->isSealed = true;

    // Resolve each phi by looking in each predecessor for the name of the phi.
    for (auto& phi : block->incompletePhis) {
        for (auto pred : block->predecessors) {
            auto predValue = findName(context, method, phi->value.name, pred);

            assert(predValue != hir::kInvalidNVID);
            assert(block->frame->values[predValue]);

            phi->addInput(block->frame->values[predValue]);
        }
    }

    block->phis.splice(block->phis.end(), block->incompletePhis);
}

hir::NVID BlockBuilder::append(std::unique_ptr<hir::HIR> hir, Block* block) {
    return insert(std::move(hir), block, block->statements.end());
}

hir::NVID BlockBuilder::insert(std::unique_ptr<hir::HIR> hir, Block* block,
        std::list<std::unique_ptr<hir::HIR>>::iterator before) {
    // Phis should only be inserted by findScopedName().
    assert(hir->opcode != hir::Opcode::kPhi);

    auto valueNumber = static_cast<hir::NVID>(block->frame->values.size());
    auto value = hir->proposeValue(valueNumber);
    // We don't bump the value serial for invalid values (meaning read-only operations)
    if (value != hir::kInvalidNVID) {
        block->frame->values.emplace_back(hir.get());
    }

    hir->owningBlock = block;

    // Update the producers of values this hir consumes.
    for (auto id : hir->reads) {
        assert(block->frame->values[id]);
        block->frame->values[id]->consumers.insert(hir.get());
    }

    block->statements.emplace(before, std::move(hir));
    return value;
}

hir::NVID BlockBuilder::findName(ThreadContext* context, const library::Method method, library::Symbol name,
        Block* block) {

    // If this symbol defines a class name look it up in the class library and provide it as a constant.
    if (name.isClassName(context)) {
        auto classDef = context->classLibrary->findClassNamed(name);
        assert(!classDef.isNil());
        auto constant = std::make_unique<hir::ConstantHIR>(classDef.slot(), name);
        constant->value.knownClassName = name;
        hir::NVID nodeValue = append(std::move(constant), block);
        block->revisions.emplace(std::make_pair(name, nodeValue));
        return nodeValue;
    }

    // Search through local values, including variables, arguments, and already-cached imports, for a name.
    Block* searchBlock = block;
    std::list<Block*> outerBlocks;
 
    while (searchBlock) {
        hir::NVID nodeValue = findScopedName(context, name, searchBlock);

        if (nodeValue != hir::kInvalidNVID) {
            // If we found this value in a external frame we will need to add import statements to each frame between
            // this block and the importing block, starting with the outermost frame that didn't have the value.
            for (auto outerBlock : outerBlocks) {
                auto import = std::make_unique<hir::ImportLocalVariableHIR>(name,
                        outerBlock->frame->outerBlock->frame->values[nodeValue]->value.typeFlags, nodeValue);

                // Insert just after the branch statement at the end of the import block.
                auto iter = outerBlock->frame->rootScope->blocks.front()->statements.end();
                --iter;
                nodeValue = insert(std::move(import), outerBlock, iter);
                outerBlock->frame->rootScope->blocks.front()->revisions.emplace(std::make_pair(name, nodeValue));

                // Now plumb that value through to the block containing the inner frame.
                nodeValue = findScopedName(context, name, outerBlock);
            }

            assert(block->scope->frame->values[nodeValue]);
            return nodeValue;
        }

        outerBlocks.emplace_front(searchBlock);

        // Search in the next outside block, if one exists.
        searchBlock = searchBlock->frame->outerBlock;
    }

    // The next several options will all require an import, so set up the iterator for HIR insertion to point at the
    // branch instruction in the import block, so that the imported HIR will insert right before it.
    hir::NVID nodeValue = hir::kInvalidNVID;
    auto importIter = block->frame->rootScope->blocks.front()->statements.end();
    --importIter;

    // Search instance variables next.
    library::Class classDef = method.ownerClass();
    auto className = classDef.name(context).view(context);
    bool isMetaClass = (className.size() > 5) && (className.substr(0, 5) == "Meta_");

    // Meta_ classes are descended from Class, so don't have access to these regular instance variables, so skip the
    // search for instance variable names in that case.
    if (!isMetaClass) {
        auto instVarIndex = classDef.instVarNames().indexOf(name);
        if (instVarIndex.isInt32()) {
            auto thisValue = findName(context, method, context->symbolTable->thisSymbol(), block);
            assert(thisValue != hir::kInvalidNVID);
            nodeValue = insert(std::make_unique<hir::ImportInstanceVariableHIR>(name, thisValue,
                    instVarIndex.getInt32()), block->frame->rootScope->blocks.front().get(), importIter);
        }
    } else {
        // Meta_ classes to have access to the class variables and constants of their associated class, so adjust the
        // classDef to point to the associated class instead.
        className = className.substr(5);
        classDef = context->classLibrary->findClassNamed(library::Symbol::fromView(context, className));
        assert(!classDef.isNil());
    }

    // Search class variables next, starting from this class and up through all parents.
    if (nodeValue == hir::kInvalidNVID) {
        library::Class classVarDef = classDef;

        while (!classVarDef.isNil()) {
            auto classVarOffset = classVarDef.classVarNames().indexOf(name);
            if (classVarOffset.isInt32()) {
                nodeValue = insert(std::make_unique<hir::ImportClassVariableHIR>(name, classVarDef,
                        classVarOffset.getInt32()), block->frame->rootScope->blocks.front().get(), importIter);
                break;
            }

            classVarDef = context->classLibrary->findClassNamed(classVarDef.superclass(context));
        }
    }

    // Search constants next.
    if (nodeValue == hir::kInvalidNVID) {
        library::Class classConstDef = classDef;

        while (!classConstDef.isNil()) {
            auto constIndex = classConstDef.constNames().indexOf(name);
            if (constIndex.isInt32()) {
                // We still put constants in the import block to avoid them being undefined along any path in the CFG.
                nodeValue = insert(std::make_unique<hir::ConstantHIR>(
                        classDef.constValues().at(constIndex.getInt32()), name),
                        block->frame->rootScope->blocks.front().get(), importIter);
                break;
            }

            classConstDef = context->classLibrary->findClassNamed(classConstDef.superclass(context));
        }
    }

    // If we found a match we've inserted it into the import block. Use findScopedName to set up all the phis
    // and local value mappings between the import block and the current block.
    if (nodeValue != hir::kInvalidNVID) {
        assert(block->frame->rootScope->blocks.front()->revisions.find(name) ==
               block->frame->rootScope->blocks.front()->revisions.end());
        block->frame->rootScope->blocks.front()->revisions.emplace(std::make_pair(name, nodeValue));

        auto foundValue = findScopedName(context, name, block);
        return foundValue;
    }

    // Check for special names.
    if (name == context->symbolTable->superSymbol()) {
        auto thisValue = findName(context, method, context->symbolTable->thisSymbol(), block);
        assert(thisValue != hir::kInvalidNVID);
        nodeValue = append(std::make_unique<hir::RouteToSuperclassHIR>(thisValue), block);
    } else if (name == context->symbolTable->thisMethodSymbol()) {
        nodeValue = append(std::make_unique<hir::ConstantHIR>(method.slot(), name), block);
    } else if (name == context->symbolTable->thisProcessSymbol()) {
        nodeValue = append(std::make_unique<hir::ConstantHIR>(Slot::makePointer(context->thisProcess), name), block);
    } else if (name == context->symbolTable->thisThreadSymbol()) {
        nodeValue = append(std::make_unique<hir::ConstantHIR>(Slot::makePointer(context->thisThread), name), block);
    }

    // Special names can all be appended locally to the block, no import required
    if (nodeValue != hir::kInvalidNVID) {
        block->revisions.emplace(std::make_pair(name, nodeValue));
    } else {
        SPDLOG_CRITICAL("Failed to find name: {}", name.view(context));
    }

    return nodeValue;
}

hir::NVID BlockBuilder::findScopedName(ThreadContext* context, library::Symbol name, Block* block) {
    std::unordered_map<int32_t, hir::NVID> blockValues;

    std::unordered_set<const Scope*> containingScopes;
    const Scope* scope = block->scope;
    while (scope) {
        containingScopes.emplace(scope);
        scope = scope->parent;
    }

    std::unordered_map<hir::HIR*, hir::HIR*> trivialPhis;

    auto value = findScopedNameRecursive(context, name, block, blockValues, containingScopes, trivialPhis);
    if (value == hir::kInvalidNVID) { return hir::kInvalidNVID; }

    while (trivialPhis.size()) {
        replaceValues(trivialPhis);
        trivialPhis.clear();
        blockValues.clear();
        value = findScopedNameRecursive(context, name, block, blockValues, containingScopes, trivialPhis);
        assert(value != hir::kInvalidNVID);
    }

    return value;
}

hir::NVID BlockBuilder::findScopedNameRecursive(ThreadContext* context, library::Symbol name, Block* block,
        std::unordered_map<Block::ID, hir::NVID>& blockValues,
        const std::unordered_set<const Scope*>& containingScopes,
        std::unordered_map<hir::HIR*, hir::HIR*>& trivialPhis) {

    // To avoid any infinite cycles in block loops, return any value found on a previous call on this block.
    auto cacheIter = blockValues.find(block->id);
    if (cacheIter != blockValues.end()) {
        return cacheIter->second;
    }

    // This scope is *shadowing* the variable name if the scope has a variable of the same name and is not within
    // the scope heirarchy of the search, so we should ignore any local revisions.
    bool isShadowed = (block->scope->variableNames.count(name) > 0) && (containingScopes.count(block->scope) == 0);
    assert(!isShadowed);

    auto iter = block->revisions.find(name);
    if (iter != block->revisions.end()) {
        if (!isShadowed) {
            return iter->second;
        }
    }

    // Unsealed blocks always create phis, because we can't search the complete list of predecessors.
    if (!block->isSealed) {
        auto phi = std::make_unique<hir::PhiHIR>(name);
        auto phiValue = phi->proposeValue(static_cast<hir::NVID>(block->frame->values.size()));
        phi->owningBlock = block;
        block->frame->values.emplace_back(phi.get());
        blockValues.emplace(std::make_pair(block->id, phiValue));
        block->revisions[name] = phiValue;
        block->incompletePhis.emplace_back(std::move(phi));
        return phiValue;
    }

    // Don't bother with phi creation if we have no or only one predecessor.
    if (block->predecessors.size() == 0) {
        blockValues.emplace(std::make_pair(block->id, hir::kInvalidNVID));
        return hir::kInvalidNVID;
    } else if (block->predecessors.size() == 1) {
        auto foundValue = findScopedNameRecursive(context, name, block->predecessors.front(), blockValues,
                containingScopes, trivialPhis);
        blockValues.emplace(std::make_pair(block->id, foundValue));
        return foundValue;
    }

    // Either no local revision found or the local revision is ignored, we need to search recursively upward. Create
    // a phi for possible insertion into the local map (if not shadowed).
    auto phi = std::make_unique<hir::PhiHIR>(name);
    auto phiValue = phi->proposeValue(static_cast<hir::NVID>(block->frame->values.size()));
    phi->owningBlock = block;
    block->frame->values.emplace_back(phi.get());
    blockValues.emplace(std::make_pair(block->id, phiValue));

    // Search predecessors for the name.
    for (auto pred : block->predecessors) {
        auto foundValue = findScopedNameRecursive(context, name, pred, blockValues, containingScopes, trivialPhis);

        // This is a depth-first search, so the above recursive call will return after searching up until either the
        // name is found or the import block (with no predecessors) is found empty. So an invalidNVID return here means
        // we have not found the name in any scope along the path from here to root, and need to clean up the phi and
        // return kInvalidNVID.
        if (foundValue == hir::kInvalidNVID) {
            assert(phi->reads.size() == 0);
            assert(trivialPhis.size() == 0);
            block->frame->values[phiValue] = nullptr;
            blockValues[block->id] = hir::kInvalidNVID;
            return hir::kInvalidNVID;
        }

        assert(foundValue < static_cast<hir::NVID>(block->frame->values.size()));
        assert(block->frame->values[foundValue]);
        phi->addInput(block->frame->values[foundValue]);
    }
    assert(phi->inputs.size());

    auto phiRef = phi.get();
    block->phis.emplace_back(std::move(phi));

    auto trivialValue = phiRef->getTrivialValue();
    if (trivialValue == hir::kInvalidNVID) {
        block->revisions[name] = phiValue;
        return phiValue;
    }

    // This phi may be trivial, but because the graph has possible cycles with loops it may already be in other phis or
    // even statements in successors (who are also predecessors). Remove if so, and all other phis made trivial by this
    // substitution.
    blockValues[block->id] = trivialValue;
    block->revisions[name] = trivialValue;

    assert(block->frame->values[trivialValue]);
    trivialPhis.emplace(std::make_pair(phiRef, block->frame->values[trivialValue]));

    return trivialValue;
}

void BlockBuilder::replaceValues(std::unordered_map<hir::HIR*, hir::HIR*>& replacements) {
    std::unordered_set<hir::HIR*> toRemove;

    while (replacements.size()) {
        auto iter = replacements.begin();
        auto orig = iter->first;
        assert(orig->value.id != hir::kInvalidNVID);

        auto repl = iter->second;
        assert(repl->value.id != hir::kInvalidNVID);

        replacements.erase(iter);

        // Update all consumers of this value with the replacement value.
        for (auto consumer : orig->consumers) {
            if (consumer == orig) { continue; }

            consumer->replaceInput(orig->value.id, repl->value.id);

            // Alter any block-local name revisions that may be caching this value.
            if (!orig->value.name.isNil()) {
                auto revisionIter = consumer->owningBlock->revisions.find(orig->value.name);
                if (revisionIter != consumer->owningBlock->revisions.end() && revisionIter->second == orig->value.id) {
                    revisionIter->second = repl->value.id;
                }
            }

            // Phis may themselves be rendered trivial by this replacement, if they are we append them to the list
            // of replacements.
            if (consumer->opcode == hir::Opcode::kPhi && toRemove.count(consumer) == 0) {
                auto phi = reinterpret_cast<hir::PhiHIR*>(consumer);
                auto trivialValue = phi->getTrivialValue();
                if (trivialValue != hir::kInvalidNVID) {
                    replacements[consumer] = consumer->owningBlock->frame->values[trivialValue];
                }
            }
        }

        if (!orig->value.name.isNil()) {
            auto revisionIter = orig->owningBlock->revisions.find(orig->value.name);
            if (revisionIter != orig->owningBlock->revisions.end() && revisionIter->second == orig->value.id) {
                revisionIter->second = repl->value.id;
            }

            revisionIter = repl->owningBlock->revisions.find(orig->value.name);
            if (revisionIter != repl->owningBlock->revisions.end() && revisionIter->second == orig->value.id) {
                revisionIter->second = repl->value.id;
            }
        }

        toRemove.emplace(orig);
    }

    for (auto orig : toRemove) {
        // Now delete the original from its block.
        orig->owningBlock->frame->values[orig->value.id] = nullptr;

        if (orig->opcode == hir::Opcode::kPhi) {
            for (auto iter = orig->owningBlock->phis.begin(); iter != orig->owningBlock->phis.end(); ++iter) {
                if (orig->value.id == (*iter)->value.id) {
                    orig->owningBlock->phis.erase(iter);
                    break;
                }
            }
        } else {
            for (auto iter = orig->owningBlock->statements.begin(); iter != orig->owningBlock->statements.end();
                    ++iter) {
                if (orig->value.id == (*iter)->value.id) {
                    orig->owningBlock->statements.erase(iter);
                    break;
                }
            }
        }
    }
}

} // namespace hadron