#include "hadron/BlockBuilder.hpp"

#include "hadron/AST.hpp"
#include "hadron/Block.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/LinearBlock.hpp"
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

std::unique_ptr<Frame> BlockBuilder::buildFrame(ThreadContext* context, const ast::BlockAST* blockAST) {
    // Build outer frame, root scope, and entry block.
    auto frame = std::make_unique<Frame>();

    frame->argumentOrder = blockAST->argumentNames;
    frame->argumentDefaults = blockAST->argumentDefaults;

    frame->rootScope = std::make_unique<Scope>(frame.get());
    auto scope = frame->rootScope.get();
    scope->blocks.emplace_back(std::make_unique<Block>(scope, frame->numberOfBlocks));
    ++frame->numberOfBlocks;
    auto block = scope->blocks.front().get();

    for (int32_t argIndex = 0; argIndex < frame->argumentOrder.size(); ++argIndex) {
        auto name = frame->argumentOrder.at(argIndex);
        block->revisions[name] = std::make_pair(
                insert(std::make_unique<hir::LoadArgumentHIR>(argIndex), block),
                insert(std::make_unique<hir::LoadArgumentTypeHIR>(argIndex), block));
    }

    Block* currentBlock = block;
    block->finalValue = buildFinalValue(context, currentBlock, blockAST->statements.get());
    return frame;
}

std::unique_ptr<Scope> BlockBuilder::buildInlineBlock(ThreadContext* context, Block* predecessor,
        const ast::BlockAST* blockAST) {
    auto scope = std::make_unique<Scope>(predecessor->scope);
    scope->blocks.emplace_back(std::make_unique<Block>(scope, scope->frame->numberOfBlocks));
    ++scope->frame->numberOfBlocks;
    auto block = scope->blocks.front().get();
    block->predecessors.emplace_back(predecessor);

    // Inline blocks can have arguments but they are treated as constants with their default values.
    for (int32_t argIndex = 0; argIndex < blockAST->argumentNames.size(); ++argIndex) {
        auto name = blockAST->argumentNames.at(argIndex);
        auto value = blockAST->argumentDefaults.at(argIndex);
        block->revisions[name] = std::make_pair(
                insert(std::make_unique<hir::ConstantHIR>(value), block),
                insert(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(value.getType()))));
    }

    Block* currentBlock = block;
    block->finalValue = buildFinalValue(context, currentBlock, blockAST->statements.get());
    return scope;
}

std::pair<Value, Value> BlockBuilder::buildValue(ThreadContext* context, Block*& currentBlock, const ast::AST* ast) {
    std::pair<Value, Value> nodeValue;

    switch(ast->astType) {
    case ast::ASTType::kEmpty:
        nodeValue.first = insert(std::make_unique<hir::ConstantHIR>(Slot::makeNil()), currentBlock);
        nodeValue.second = insert(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(Type::kNil)), currentBlock);
        break;

    case ast::ASTType::kSequence: {
        const auto seq = reinterpret_cast<const ast::SequenceAST*>(ast);
        nodeValue = buildFinalValue(context, currentBlock, seq);
    } break;

    // Blocks inline are block literals. We need to compile the AST down to bytecode, then in this context the
    // finalValue is a pointer back to the Function object which references the compiled FunctionDef.
    case ast::ASTType::kBlock: {
        assert(false);
    } break;

    case ast::ASTType::kIf: {
        const auto ifAST = reinterpret_cast<const ast::IfAST*>(ast);
        nodeValue = buildIf(context, currentBlock, ifAST);
    } break;

    case ast::ASTType::kMessage: {
        const auto message = reinterpret_cast<const ast::MessageAST*>(ast);
        // Build argument values starting with target argument as `this`.
        std::vector<std::pair<Value, Value>> argumentValues;
        argumentValues.reserve(message->arguments->sequence.size() + 1);
        argumentValues.emplace_back(buildValue(context, currentBlock, message->target.get()));

        auto selectorValue = std::make_pair(
                insert(std::make_unique<hir::ConstantHIR>(message->selector.slot()), currentBlock),
                insert(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(Type::kSymbol)), currentBlock));

        // Append any additional arguments.
        for (const auto& arg : message->arguments->sequence) {
            argumentValues.emplace_back(buildValue(context, currentBlock, arg.get()));
        }

        // Build keyword argument pairs.
        std::vector<std::pair<Value, Value>> keywordArgumentValues;
        keywordArgumentValues.reserve(message->keywordArguments->sequence.size());
        for (const auto& arg : message->keywordArguments->sequence) {
            keywordArgumentValues.emplace_back(buildValue(context, currentBlock, arg.get()));
        }

        nodeValue = buildDispatch(selectorValue, argumentValues, keywordArgumentValues);
    } break;

    case ast::ASTType::kName: {
        const auto nameAST = reinterpret_cast<const ast::NameAST*>(ast);
        nodeValue = findName(nameAST->name);
    } break;

    case ast::ASTType::kAssign: {
        const auto assign = reinterpret_cast<const ast::AssignAST*>(ast);
        nodeValue = buildValue(context, currentBlock, assign->value.get());
        currentBlock->revisions[assign->name->name] = nodeValue;
    } break;

    case ast::ASTType::kConstant: {
        const auto constAST = reinterpret_cast<const ast::ConstantAST*>(ast);
        nodeValue.first = insert(std::make_unique<hir::ConstantHIR>(constAST->constant), currentBlock);
        nodeValue.second = insert(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(constAST->constant.getType())),
                currentBlock);
    } break;

    // ================ BROKEN LINE

    case parse::NodeType::kReturn: {
        const auto returnNode = reinterpret_cast<const parse::ReturnNode*>(node);
        assert(returnNode->valueExpr);
        nodeValue = buildFinalValue(context, returnNode->valueExpr.get());
        insertLocal(std::make_unique<hir::StoreReturnHIR>(nodeValue));
    } break;

    case parse::NodeType::kList: {
        // Create a new array with a dispatch to Array.new()
        auto selectorValue = std::make_pair(
            insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeHash(hash("new")))),
            insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(Type::kSymbol))));
        std::vector<std::pair<Value, Value>> argumentValues;
        // TODO: This is broken! Need to be able to look up Meta_Array in the Class Library, not passing the
        // symbol to it here but the actual target class.
        argumentValues.emplace_back(std::make_pair(
            insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeNil())),
            selectorValue.second));
        std::vector<std::pair<Value, Value>> keywordArgumentValues;
        nodeValue = buildDispatchInternal(selectorValue, argumentValues, keywordArgumentValues);

        // Evaluate each element in the parse tree in turn and append to list with a call to array.add()
        selectorValue.first = insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeHash(hash("add"))));

        const auto list = reinterpret_cast<const parse::ListNode*>(node);
        const parse::ExprSeqNode* element = list->elements.get();
        while (element) {
            argumentValues.clear();
            argumentValues.emplace_back(nodeValue);
            argumentValues.emplace_back(buildFinalValue(context, element));
            nodeValue = buildDispatchInternal(selectorValue, argumentValues, keywordArgumentValues);

            element = reinterpret_cast<const parse::ExprSeqNode*>(element->next.get());
        }
    } break;

    case parse::NodeType::kDictionary: {
        assert(false);
    } break;



    case parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const parse::LiteralNode*>(node);
        nodeValue.first = insertLocal(std::make_unique<hir::ConstantHIR>(literal->value));
        nodeValue.second = insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(literal->type)));
    } break;

    case parse::NodeType::kName: {
        const auto nameNode = reinterpret_cast<const parse::NameNode*>(node);
        auto name = library::Symbol::fromView(context, m_lexer->tokens()[nameNode->tokenIndex].range);
        nodeValue = findName(name);
    } break;

    case parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(node);
        assert(exprSeq->expr);
        nodeValue = buildFinalValue(context, exprSeq->expr.get());
    } break;

    case parse::NodeType::kAssign: {
        const auto assign = reinterpret_cast<const parse::AssignNode*>(node);
        assert(assign->name);
        assert(assign->value);
        nodeValue = buildFinalValue(context, assign->value.get());
        auto name = library::Symbol::fromView(context, m_lexer->tokens()[assign->name->tokenIndex].range);
        m_block->revisions[name] = nodeValue;
    } break;

    case parse::NodeType::kSetter: {
        const auto setter = reinterpret_cast<const parse::SetterNode*>(node);
        assert(setter->target);
        assert(setter->value);
        // Rehash selector with the _ character appended.
        auto selectorToken = m_lexer->tokens()[setter->tokenIndex];
        auto selector = library::Symbol::fromView(context, fmt::format("{}_", selectorToken.range));
        nodeValue = buildDispatch(context, setter->target.get(), selector, setter->value.get(), nullptr);
    } break;

    case parse::NodeType::kKeyValue:
        assert(false);  // Top-level keyvalue pair is a syntax error.
        break;

    case parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const parse::CallNode*>(node);
        auto selector = library::Symbol::fromView(context, m_lexer->tokens()[call->tokenIndex].range);
        nodeValue = buildDispatch(context, call->target.get(), selector, call->arguments.get(),
                call->keywordArguments.get());
    } break;

    case parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(node);
        auto selector = library::Symbol::fromView(context, m_lexer->tokens()[binop->tokenIndex].range);
        nodeValue = buildDispatch(context, binop->leftHand.get(), selector, binop->rightHand.get(), nullptr);
    } break;

    }

    nodeValue.first = findValue(nodeValue.first);
    nodeValue.second = findValue(nodeValue.second);
    return nodeValue;
}

std::pair<Value, Value> BlockBuilder::buildFinalValue(ThreadContext* context, const parse::Node* node) {
    std::pair<Value, Value> finalValue;
    while (node) {
        m_block->finalValue = buildValue(context, node);
        node = node->next.get();
    }
    return m_block->finalValue;
}

std::pair<Value, Value> BlockBuilder::buildDispatch(ThreadContext* context, const parse::Node* target,
        library::Symbol selector, const parse::Node* arguments, const parse::KeyValueNode* keywordArguments) {

}

std::pair<Value, Value> BlockBuilder::buildDispatchInternal(std::pair<Value, Value> selectorValue,
        const std::vector<std::pair<Value, Value>>& argumentValues,
        const std::vector<std::pair<Value, Value>>& keywordArgumentValues) {
    // Setup the stack for the dispatch.
    insertLocal(std::make_unique<hir::DispatchSetupStackHIR>(selectorValue, argumentValues.size(),
            keywordArgumentValues.size() / 2));
    for (int i = 0; i < static_cast<int>(argumentValues.size()); ++i) {
        insertLocal(std::make_unique<hir::DispatchStoreArgHIR>(i, argumentValues[i]));
    }
    for (int i = 0; i < static_cast<int>(keywordArgumentValues.size()); i += 2) {
        insertLocal(std::make_unique<hir::DispatchStoreKeyArgHIR>(i / 2, keywordArgumentValues[i],
                keywordArgumentValues[i + 1]));
    }

    // Make the call, this will mark all registers as blocked.
    insertLocal(std::make_unique<hir::DispatchCallHIR>());

    Value returnValue = insertLocal(std::make_unique<hir::DispatchLoadReturnHIR>());
    Value returnValueType = insertLocal(std::make_unique<hir::DispatchLoadReturnTypeHIR>());
    insertLocal(std::make_unique<hir::DispatchCleanupHIR>());
    return std::make_pair(returnValue, returnValueType);
}

std::pair<Value, Value> BlockBuilder::buildIf(ThreadContext* context, Block*& currentBlock, const ast::IfAST* ifAST) {
    std::pair<Value, Value> nodeValue;
    // Compute final value of the condition.
    auto condition = buildFinalValue(context, currentBlock, ifAST->condition.get());

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
    auto trueScopeOwning = buildInlineBlock(context, conditionBlock, ifAST->trueBlock.get());
    Scope* trueScope = trueScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(trueScopeOwning));
    trueBranch->blockNumber = trueScope->blocks.front()->number;
    conditionBlock->successors.emplace_back(trueScope->blocks.front().get());

    // Build the false condition block.
    auto falseScopeOwning = buildInlineBlock(context, conditionBlock, ifAST->falseBlock.get());
    Scope* falseScope = falseScopeOwning.get();
    parentScope->subScopes.emplace_back(std::move(falseScopeOwning));
    falseBranch->blockNumber = falseScope->blocks.front()->number;
    conditionBlock->successors.emplace_back(falseScope->blocks.front().get());

    // Create a new block in the parent frame for code after the if statement.
    auto continueBlock = std::make_unique<Block>(parentScope, parentScope->frame->numberOfBlocks);
    ++parentScope->frame->numberOfBlocks;
    currentBlock = continueBlock.get();
    parentScope->blocks.emplace_back(std::move(continueBlock));

    // Wire trueScope exit block to the continue block here.
    auto exitTrueBranch = std::make_unique<hir::BranchHIR>();
    exitTrueBranch->blockNumber = currentBlock->number;
    insert(std::move(exitTrueBranch), trueScope->blocks.back().get());
    trueScope->blocks.back()->successors.emplace_back(currentBlock);
    currentBlock->predecessors.emplace_back(trueScope->blocks.back().get());

    // Wire falseFrame exit block to the continue block here.
    auto exitFalseBranch = std::make_unique<hir::BranchHIR>();
    exitFalseBranch->blockNumber = currentBlock->number;
    insert(std::move(exitFalseBranch), falseScope->blocks.back().get());
    falseScope->blocks.back()->successors.emplace_back(currentBblock);
    currentBlock->predecessors.emplace_back(falseScope->blocks.back().get());

    // Add phis with the final values of both the false and true values here, this is also the value of the
    // if statement.
    auto valuePhi = std::make_unique<hir::PhiHIR>();
    valuePhi->addInput(trueScope->blocks.back()->finalValue.first);
    valuePhi->addInput(falseScope->blocks.back()->finalValue.first);
    nodeValue.first = valuePhi->proposeValue(currentBlock->scope->frame->numberOfValues);
    ++currentBlock->scope->frame->numberOfValues;
    currentBlock->localValues.emplace(std::make_pair(nodeValue.first, nodeValue.first));
    currentBlock->phis.emplace_back(std::move(valuePhi));

    auto typePhi = std::make_unique<hir::PhiHIR>();
    typePhi->addInput(trueScope->blocks.back()->finalValue.second);
    typePhi->addInput(falseScope->blocks.back()->finalValue.second);
    nodeValue.second = typePhi->proposeValue(currentBlock->scope->frame->numberOfValues);
    ++currentBlock->scope->frame->numberOfValues;
    currentBlock->localValues.emplace(std::make_pair(nodeValue.second, nodeValue.second));
    currentBlock->phis.emplace_back(std::move(typePhi));

    return nodeValue;
}

Value BlockBuilder::insert(std::unique_ptr<hir::HIR> hir, Block* block) {
    // Phis should only be inserted by findValuePredecessor().
    assert(hir->opcode != hir::Opcode::kPhi);
    auto valueNumber = block->scope->frame->numberOfValues;
    auto value = hir->proposeValue(valueNumber);
    // We don't bump the value serial for invalid values (meaning read-only operations)
    if (value.isValid()) {
        ++block->scope->frame->numberOfValues;
        block->values.emplace(std::make_pair(value, hir.get()));
        block->localValues.emplace(std::make_pair(value, value));
    }
    block->statements.emplace_back(std::move(hir));
    return value;
}

std::pair<Value, Value> BlockBuilder::findName(library::Symbol name) {
    std::unordered_map<int, std::pair<Value, Value>> blockValues;
    std::unordered_set<const Scope*> containingScopes;
    const Scope* scope = m_block->scope;
    while (scope) {
        containingScopes.emplace(scope);
        scope = scope->parent;
    }
    return findNamePredecessor(name, m_block, blockValues, containingScopes);
}

std::pair<Value, Value> BlockBuilder::findNamePredecessor(library::Symbol name, Block* block,
        std::unordered_map<int, std::pair<Value, Value>>& blockValues,
        const std::unordered_set<const Scope*>& containingScopes) {
    auto cacheIter = blockValues.find(block->number);
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
    // a pair of phis for possible insertion into the local map (if not shadowed).
    auto phiForValue = std::make_unique<hir::PhiHIR>();
    auto phiForValueValue = phiForValue->proposeValue(m_valueSerial);
    ++m_valueSerial;
    auto phiForType = std::make_unique<hir::PhiHIR>();
    auto phiForTypeValue = phiForType->proposeValue(m_valueSerial);
    ++m_valueSerial;
    auto phiValues = std::make_pair(phiForValueValue, phiForTypeValue);
    blockValues.emplace(std::make_pair(block->number, phiValues));

    // Search predecessors for the name.
    for (auto pred : block->predecessors) {
        auto foundValues = findNamePredecessor(name, pred, blockValues, containingScopes);
        phiForValue->addInput(foundValues.first);
        phiForType->addInput(foundValues.second);
    }

    // TODO: It's possible that phis have 0 inputs here, meaning you'll get an assert on phiForValue->getTrivialValue.
    // That means the name is undefined, and we should return an error.

    auto valueTrivial = phiForValue->getTrivialValue();
    auto typeTrivial = phiForType->getTrivialValue();

    // Shadowed variables should always result in trivial phis, due to the fact that Scopes must always have exactly
    // one entry Block with at most one predecessor. The local Scope is shadowing the name we're looking for, and
    // so won't modify the unshadowed value, meaning the value of the name at entry to the Scope should be the same
    // throughout every Block in the scope.
    if (isShadowed) {
        assert(valueTrivial.isValid());
        assert(typeTrivial.isValid());
        auto trivialPair = std::make_pair(valueTrivial, typeTrivial);
        // Overwrite block values with the trivial values.
        blockValues.emplace(std::make_pair(block->number, trivialPair));
        return trivialPair;
    }

    std::pair<Value, Value> finalValues;
    if (valueTrivial.isValid()) {
        finalValues.first = valueTrivial;
    } else {
        finalValues.first = phiValues.first;
        block->phis.emplace_back(std::move(phiForValue));
    }
    if (typeTrivial.isValid()) {
        finalValues.second = typeTrivial;
    } else {
        finalValues.second = phiValues.second;
        block->phis.emplace_back(std::move(phiForType));
    }

    // Update block revisions and the blockValues map with the final values.
    block->revisions.emplace(name, finalValues);
    blockValues.emplace(std::make_pair(block->number, finalValues));

    // TODO: what happens to local values? Shouldn't need to update local value map, or should we?

    return finalValues;
}

Value BlockBuilder::findValue(Value v) {
    std::unordered_map<int, Value> blockValues;
    return findValuePredecessor(v, m_block, blockValues);
}

Value BlockBuilder::findValuePredecessor(Value v, Block* block, std::unordered_map<int, Value>& blockValues) {
    auto cacheIter = blockValues.find(block->number);
    if (cacheIter != blockValues.end()) {
        return cacheIter->second;
    }

    // Quick check if value exists in local block lookup already.
    auto localIter = block->localValues.find(v);
    if (localIter != block->localValues.end()) {
        return localIter->second;
    }

    // We make a temporary phi with a unique value but we do not put it into the local values map yet. This prevents
    // infinite recursion loops when traversing backedges in the control flow graph.
    auto phi = std::make_unique<hir::PhiHIR>();
    auto phiValue = phi->proposeValue(m_valueSerial);
    ++m_valueSerial;
    blockValues.emplace(std::make_pair(block->number, phiValue));

    // Recursive search through predecessors for values.
    for (auto pred : block->predecessors) {
        phi->addInput(findValuePredecessor(v, pred, blockValues));
    }

    // If the phi is trivial we use the trivial value directly. If not, insert it into the phi list.
    auto trivialValue = phi->getTrivialValue();
    if (trivialValue.isValid()) {
        block->localValues.emplace(std::make_pair(v, trivialValue));
        // Overwrite this block's blockValue with the updated trivial value.
        blockValues.emplace(std::make_pair(block->number, trivialValue));
        return trivialValue;
    }

    // Nontrivial phi, add it to local values, phis, and return.
    block->localValues.emplace(std::make_pair(v, phiValue));
    block->phis.emplace_back(std::move(phi));
    return phiValue;
}

} // namespace hadron