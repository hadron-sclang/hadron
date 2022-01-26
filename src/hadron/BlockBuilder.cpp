#include "hadron/BlockBuilder.hpp"

#include "hadron/Block.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Scope.hpp"
#include "hadron/Slot.hpp"
#include "hadron/ThreadContext.hpp"

#include "schema/Common/Core/Object.hpp"
#include "schema/Common/Core/Kernel.hpp"
#include "schema/Common/Collections/Collection.hpp"
#include "schema/Common/Collections/SequenceableCollection.hpp"
#include "schema/Common/Collections/ArrayedCollection.hpp"
#include "schema/Common/Collections/Array.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <cassert>
#include <string>

namespace hadron {

BlockBuilder::BlockBuilder(const Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(lexer),
    m_errorReporter(errorReporter),
    m_frame(nullptr),
    m_scope(nullptr),
    m_block(nullptr),
    m_blockSerial(0),
    m_valueSerial(0) { }

BlockBuilder::~BlockBuilder() { }

std::unique_ptr<Frame> BlockBuilder::buildFrame(ThreadContext* context, const parse::BlockNode* blockNode) {
    // Build outer frame, root scope, and entry block.
    auto frame = std::make_unique<Frame>();
    m_frame = frame.get();
    frame->argumentOrder = reinterpret_cast<library::Array*>(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    frame->argumentDefaults = reinterpret_cast<library::Array*>(context->heap->allocateObject(library::kArrayHash,
            sizeof(library::Array)));
    frame->rootScope = std::make_unique<Scope>(m_frame, nullptr);
    m_scope = frame->rootScope.get();
    m_scope->blocks.emplace_back(std::make_unique<Block>(m_scope, m_blockSerial));
    ++m_blockSerial;
    m_block = m_scope->blocks.front().get();

    // First block within rootScope gets argument loads. The *this* pointer is always the first argument to every Frame.
    frame->argumentOrder = reinterpret_cast<library::Array*>(library::ArrayedCollection::_ArrayAdd(context,
            Slot::makePointer(frame->argumentOrder), Slot::makeHash(kThisHash)).getPointer());
    frame->argumentDefaults = reinterpret_cast<library::Array*>(library::ArrayedCollection::_ArrayAdd(context,
            Slot::makePointer(frame->argumentDefaults), Slot::makeNil()).getPointer());
    m_block->revisions[kThisHash] = std::make_pair(
            insertLocal(std::make_unique<hir::LoadArgumentHIR>(0)),
            insertLocal(std::make_unique<hir::LoadArgumentTypeHIR>(0)));

    // Build the rest of the argument order.
    const parse::ArgListNode* argList = blockNode->arguments.get();
    int argIndex = 1;
    if (argList) {
        assert(argList->nodeType == parse::NodeType::kArgList);
        const parse::VarListNode* varList = argList->varList.get();
        while (varList) {
            assert(varList->nodeType == parse::NodeType::kVarList);
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                assert(varDef->nodeType == parse::NodeType::kVarDef);
                auto name = m_lexer->tokens()[varDef->tokenIndex].hash;
                frame->argumentOrder = reinterpret_cast<library::Array*>(library::ArrayedCollection::_ArrayAdd(context,
                        Slot::makePointer(frame->argumentOrder), Slot::makeHash(name)).getPointer());
                Slot initialValue = Slot::makeNil();
                if (varDef->initialValue) {
                    if (varDef->initialValue->nodeType == parse::NodeType::kLiteral) {
                        const auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                        initialValue = literal->value;
                    } else {
                        // ** TODO: add to list for if-block processing (see below)
                    }
                }
                frame->argumentDefaults = reinterpret_cast<library::Array*>(
                        library::ArrayedCollection::_ArrayAdd(context, Slot::makePointer(frame->argumentDefaults),
                        initialValue).getPointer());
                m_block->revisions[name] = std::make_pair(
                     insertLocal(std::make_unique<hir::LoadArgumentHIR>(argIndex)),
                     insertLocal(std::make_unique<hir::LoadArgumentTypeHIR>(argIndex)));
                ++argIndex;
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        // There should be at most one arglist in a parse tree.
        assert(argList->next == nullptr);
        if (argList->varArgsNameIndex) {
            auto name = m_lexer->tokens()[argList->varArgsNameIndex.value()].hash;
            frame->hasVarArgs = true;
            frame->argumentOrder = reinterpret_cast<library::Array*>(library::ArrayedCollection::_ArrayAdd(context,
                    Slot::makePointer(frame->argumentOrder), Slot::makeHash(name)).getPointer());
            frame->argumentDefaults = reinterpret_cast<library::Array*>(library::ArrayedCollection::_ArrayAdd(context,
                    Slot::makePointer(frame->argumentDefaults), Slot::makeNil()).getPointer());
            // Type is always a kArray for variable argument lists.
            m_block->revisions[name] = std::make_pair(
                insertLocal(std::make_unique<hir::LoadArgumentHIR>(argIndex, true)),
                insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(Type::kArray))));
        }
    }

    if (blockNode->variables) {
        m_block->finalValue = buildFinalValue(context, blockNode->variables.get());
    }

    // ** TODO: If this is the entry scope for a frame, insert any non-literal value initalizations here as
    // ifNil blocks.

    if (blockNode->body) {
        m_block->finalValue = buildFinalValue(context, blockNode->body.get());
    }

    frame->numberOfBlocks = m_blockSerial;
    frame->numberOfValues = m_valueSerial;
    return frame;
}

std::unique_ptr<Scope> BlockBuilder::buildSubScope(ThreadContext* context, const parse::BlockNode* blockNode) {
    assert(blockNode);
    auto scope = std::make_unique<Scope>(m_frame, m_scope);
    m_scope = scope.get();
    // Make an entry block and add to frame.
    scope->blocks.emplace_back(std::make_unique<Block>(scope.get(), m_blockSerial));
    ++m_blockSerial;
    scope->blocks.front()->predecessors.emplace_back(m_block);
    m_block = scope->blocks.begin()->get();

    if (blockNode->variables) {
        m_block->finalValue = buildFinalValue(context, blockNode->variables.get());
    }

    if (blockNode->body) {
        m_block->finalValue = buildFinalValue(context, blockNode->body.get());
    }

    return scope;
}

std::pair<Value, Value> BlockBuilder::buildValue(ThreadContext* context, const parse::Node* node) {
    std::pair<Value, Value> nodeValue;

    switch(node->nodeType) {
    case parse::NodeType::kEmpty:
        assert(false); // kEmpty is invalid node type.
        break;

    case parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const parse::VarDefNode*>(node);
        auto nameToken = m_lexer->tokens()[varDef->tokenIndex];
        // TODO: error reporting for variable redefinition
        if (varDef->initialValue) {
            nodeValue = buildFinalValue(context, varDef->initialValue.get());
            m_block->revisions[nameToken.hash] = nodeValue;
        } else {
            nodeValue.first = insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeNil()));
            nodeValue.second = insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(Type::kNil)));
            m_block->revisions[nameToken.hash] = nodeValue;
        }
    } break;

    case parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const parse::VarListNode*>(node);
        if (varList->definitions) {
            nodeValue = buildFinalValue(context, varList->definitions.get());
        }
    } break;

    case parse::NodeType::kArgList:
    case parse::NodeType::kMethod:
    case parse::NodeType::kClassExt:
    case parse::NodeType::kClass:
        assert(false); // internal error, not a valid node within a block
        break;

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
            insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeHash(library::kMetaArrayHash))),
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

    // This only works for inline blocks <- when do these happen? Are they blockLiterals?
    case parse::NodeType::kBlock: {
        assert(false); // TODO: figure out when these happen and what's going on here.
        const auto blockNode = reinterpret_cast<const parse::BlockNode*>(node);
        // Preserve current scope and block, for wiring into the new scope and block. Encountering a block always
        // terminates the current block.
        Scope* parentScope = m_scope;
        Block* lastBlock = m_block;

        // Recursively build the subframe.
        auto subScope = buildSubScope(context, blockNode);

        // Wire the entry block in the new frame as a successor in the block graph.
        Block* frameEntryBlock = subScope->blocks.front().get();
        lastBlock->successors.emplace_back(frameEntryBlock);
        frameEntryBlock->predecessors.emplace_back(lastBlock);

        // We need a new block in the parent frame as a sucessor to the exit block from the sub frame.
        assert(m_block == subScope->blocks.back().get());
        auto parentBlock = std::make_unique<Block>(parentScope, m_blockSerial);
        ++m_blockSerial;
        Block* frameExitBlock = m_block;
        parentBlock->predecessors.emplace_back(frameExitBlock);
        frameExitBlock->successors.emplace_back(parentBlock.get());
        m_block = parentBlock.get();
        parentScope->blocks.emplace_back(std::move(parentBlock));
        parentScope->subScopes.emplace_back(std::move(subScope));
        m_scope = parentScope;
    } break;

    case parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const parse::LiteralNode*>(node);
        nodeValue.first = insertLocal(std::make_unique<hir::ConstantHIR>(literal->value));
        nodeValue.second = insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(literal->type)));
    } break;

    case parse::NodeType::kName: {
        const auto nameNode = reinterpret_cast<const parse::NameNode*>(node);
        auto name = m_lexer->tokens()[nameNode->tokenIndex].hash;
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
        auto name = m_lexer->tokens()[assign->name->tokenIndex].hash;
        m_block->revisions[name] = nodeValue;
    } break;

    case parse::NodeType::kSetter: {
        const auto setter = reinterpret_cast<const parse::SetterNode*>(node);
        assert(setter->target);
        assert(setter->value);
        // Rehash selector with the _ character appended.
        auto selectorToken = m_lexer->tokens()[setter->tokenIndex];
        auto selector = hash(fmt::format("{}_", selectorToken.range));
        nodeValue = buildDispatch(context, setter->target.get(), selector, setter->value.get(), nullptr);
    } break;

    case parse::NodeType::kKeyValue:
        assert(false);  // Top-level keyvalue pair is a syntax error.
        break;

    case parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const parse::CallNode*>(node);
        auto selector = m_lexer->tokens()[call->tokenIndex].hash;
        nodeValue = buildDispatch(context, call->target.get(), selector, call->arguments.get(),
                call->keywordArguments.get());
    } break;

    case parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(node);
        auto selector = m_lexer->tokens()[binop->tokenIndex].hash;
        nodeValue = buildDispatch(context, binop->leftHand.get(), selector, binop->rightHand.get(), nullptr);
    } break;

    case parse::NodeType::kPerformList: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kNumericSeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kCurryArgument: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kArrayRead: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kArrayWrite: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kCopySeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kNew: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kSeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kSeriesIter: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kLiteralList: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kLiteralDict: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kMultiAssignVars: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kMultiAssign: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kIf: {
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(node);
        // Compute value of the condition.
        auto condition = buildFinalValue(context, ifNode->condition.get());
        // We will always make an else condition, even if just to store the value of the block as `nil` in the default
        // case. Create a branch to jump to that else block.
        auto condBranchOwning = std::make_unique<hir::BranchIfZeroHIR>(condition);
        // Insert the if HIR but keep a copy of the pointer around to connect it to the continuation or else blocks
        hir::BranchIfZeroHIR* condBranch = condBranchOwning.get();
        nodeValue.first = insertLocal(std::move(condBranchOwning));
        condBranchOwning = nullptr;
        // Also insert a branch to the condition true block.
        auto branchOwning = std::make_unique<hir::BranchHIR>();
        hir::BranchHIR* branch = branchOwning.get();
        insertLocal(std::move(branchOwning));

        // Preserve the current block and frame for insertion of the new subframes as children.
        Scope* parentScope = m_scope;
        Block* ifBlock = m_block;

        // Build the true condition block.
        assert(ifNode->trueBlock);
        auto trueScopeOwning = buildSubScope(context, ifNode->trueBlock.get());
        Scope* trueScope = trueScopeOwning.get();
        parentScope->subScopes.emplace_back(std::move(trueScopeOwning));
        branch->blockNumber = trueScope->blocks.front()->number;
        ifBlock->successors.emplace_back(trueScope->blocks.front().get());
        trueScope->blocks.front()->predecessors.emplace_back(ifBlock);

        // Build the else condition block if present, if not build a default nil valued one. The BranchIfZero will
        // target branching here.
        Scope* falseScope = nullptr;
        if (ifNode->falseBlock) {
            auto falseScopeOwning = buildSubScope(context, ifNode->falseBlock.get());
            falseScope = falseScopeOwning.get();
            parentScope->subScopes.emplace_back(std::move(falseScopeOwning));
        } else {
            auto falseScopeOwning = std::make_unique<Scope>(m_frame, parentScope);
            falseScope = falseScopeOwning.get();
            parentScope->subScopes.emplace_back(std::move(falseScopeOwning));
            falseScope->parent = parentScope;
            auto falseBlockOwning = std::make_unique<Block>(falseScope, m_blockSerial);
            ++m_blockSerial;
            Block* falseBlock = falseBlockOwning.get();
            falseScope->blocks.emplace_back(std::move(falseBlockOwning));
            falseBlock->finalValue.first = insert(std::make_unique<hir::ConstantHIR>(Slot::makeNil()), falseBlock);
            falseBlock->finalValue.second = insert(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(Type::kNil)),
                    falseBlock);
        }
        condBranch->blockNumber = falseScope->blocks.front()->number;
        ifBlock->successors.emplace_back(falseScope->blocks.front().get());
        falseScope->blocks.front()->predecessors.emplace_back(ifBlock);

        // Create a new block in the parent frame for code after the if statement.
        auto continueBlock = std::make_unique<Block>(parentScope, m_blockSerial);
        ++m_blockSerial;
        m_block = continueBlock.get();
        parentScope->blocks.emplace_back(std::move(continueBlock));
        m_scope = parentScope;

        // Add phis with the final values of both the false and true values here, this is also the value of the
        // if statement.
        auto valuePhi = std::make_unique<hir::PhiHIR>();
        valuePhi->addInput(trueScope->blocks.back()->finalValue.first);
        valuePhi->addInput(falseScope->blocks.back()->finalValue.first);
        nodeValue.first = valuePhi->proposeValue(m_valueSerial);
        m_block->localValues.emplace(std::make_pair(nodeValue.first, nodeValue.first));
        ++m_valueSerial;
        m_block->phis.emplace_back(std::move(valuePhi));
        auto typePhi = std::make_unique<hir::PhiHIR>();
        typePhi->addInput(trueScope->blocks.back()->finalValue.second);
        typePhi->addInput(falseScope->blocks.back()->finalValue.second);
        nodeValue.second = typePhi->proposeValue(m_valueSerial);
        m_block->localValues.emplace(std::make_pair(nodeValue.second, nodeValue.second));
        ++m_valueSerial;
        m_block->phis.emplace_back(std::move(typePhi));

        // Wire trueScope exit block to the continue block here.
        auto trueBranch = std::make_unique<hir::BranchHIR>();
        trueBranch->blockNumber = m_block->number;
        insert(std::move(trueBranch), trueScope->blocks.back().get());
        trueScope->blocks.back()->successors.emplace_back(m_block);
        m_block->predecessors.emplace_back(trueScope->blocks.back().get());

        // Wire falseFrame exit block to the continue block here.
        assert(falseScope);
        auto falseBranch = std::make_unique<hir::BranchHIR>();
        falseBranch->blockNumber = m_block->number;
        insert(std::move(falseBranch), falseScope->blocks.back().get());
        falseScope->blocks.back()->successors.emplace_back(m_block);
        m_block->predecessors.emplace_back(falseScope->blocks.back().get());
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

std::pair<Value, Value> BlockBuilder::buildDispatch(ThreadContext* context, const parse::Node* target, Hash selector,
        const parse::Node* arguments, const parse::KeyValueNode* keywordArguments) {
    // Build argument values starting with target argument as `this`.
    std::vector<std::pair<Value, Value>> argumentValues;
    argumentValues.emplace_back(buildFinalValue(context, target));

    auto selectorValue = std::make_pair(
            insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeHash(selector))),
            insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeInt32(Type::kSymbol))));

    // Now append any additional arguments.
    while (arguments) {
        argumentValues.emplace_back(buildValue(context, arguments));
        arguments = arguments->next.get();
    }

    std::vector<std::pair<Value, Value>> keywordArgumentValues;
    while (keywordArguments) {
        assert(keywordArguments->nodeType == parse::NodeType::kKeyValue);
        auto keyName = m_lexer->tokens()[keywordArguments->tokenIndex].hash;
        keywordArgumentValues.emplace_back(std::make_pair(
                    insertLocal(std::make_unique<hir::ConstantHIR>(Slot::makeHash(keyName))),
                    selectorValue.second));
        keywordArgumentValues.emplace_back(buildFinalValue(context, keywordArguments->value.get()));
        keywordArguments = reinterpret_cast<const parse::KeyValueNode*>(keywordArguments->next.get());
    }
    return buildDispatchInternal(selectorValue, argumentValues, keywordArgumentValues);
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

Value BlockBuilder::insertLocal(std::unique_ptr<hir::HIR> hir) {
    return insert(std::move(hir), m_block);
}

Value BlockBuilder::insert(std::unique_ptr<hir::HIR> hir, Block* block) {
    // Phis should only be inserted by findValuePredecessor().
    assert(hir->opcode != hir::Opcode::kPhi);
    auto valueNumber = m_valueSerial;
    auto value = hir->proposeValue(valueNumber);
    // We don't bump the value serial for invalid values (meaning read-only operations)
    if (value.isValid()) {
        ++m_valueSerial;
        block->values.emplace(std::make_pair(value, hir.get()));
        block->localValues.emplace(std::make_pair(value, value));
    }
    block->statements.emplace_back(std::move(hir));
    return value;
}

std::pair<Value, Value> BlockBuilder::findName(Hash name) {
    std::unordered_map<int, std::pair<Value, Value>> blockValues;
    std::unordered_set<const Scope*> containingScopes;
    const Scope* scope = m_block->scope;
    while (scope) {
        containingScopes.emplace(scope);
        scope = scope->parent;
    }
    return findNamePredecessor(name, m_block, blockValues, containingScopes);
}

std::pair<Value, Value> BlockBuilder::findNamePredecessor(Hash name, Block* block,
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