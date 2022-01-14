#include "hadron/BlockBuilder.hpp"

#include "hadron/Block.hpp"
#include "hadron/ErrorReporter.hpp"
#include "hadron/Frame.hpp"
#include "hadron/Heap.hpp"
#include "hadron/Keywords.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/LinearBlock.hpp"
#include "hadron/Parser.hpp"
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

#include <string>

namespace hadron {

BlockBuilder::BlockBuilder(const Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(lexer),
    m_errorReporter(errorReporter),
    m_frame(nullptr),
    m_block(nullptr),
    m_blockSerial(0),
    m_valueSerial(0) { }

BlockBuilder::~BlockBuilder() { }

std::unique_ptr<Frame> BlockBuilder::buildFrame(ThreadContext* context, const parse::BlockNode* blockNode) {
    auto frame = buildSubframe(context, blockNode);
    frame->numberOfBlocks = m_blockSerial;
    frame->numberOfValues = m_valueSerial;
    return frame;
}

std::unique_ptr<Frame> BlockBuilder::buildSubframe(ThreadContext* context, const parse::BlockNode* blockNode) {
    assert(blockNode);
    auto frame = std::make_unique<Frame>();
    frame->parent = m_frame;
    m_frame = frame.get();
    // Make an entry block and add to frame.
    frame->blocks.emplace_back(std::make_unique<Block>(m_frame, m_blockSerial));
    ++m_blockSerial;
    m_block = frame->blocks.begin()->get();

    // Build argument name list and default values.
    // The *this* pointer is always the first argument to every root block.
    if (frame->parent == nullptr) {
        frame->argumentOrder = Slot(context->heap->allocateObject(library::kArrayHash, sizeof(library::Array)));
        frame->argumentOrder = library::ArrayedCollection::_ArrayAdd(context, frame->argumentOrder, kThisHash);
        frame->argumentDefaults = Slot(context->heap->allocateObject(library::kArrayHash, sizeof(library::Array)));
        frame->argumentDefaults = library::ArrayedCollection::_ArrayAdd(context, frame->argumentDefaults, Slot());
    }

    // Build the rest of the argument order.
    const parse::ArgListNode* argList = blockNode->arguments.get();
    while (argList) {
        assert(argList->nodeType == parse::NodeType::kArgList);
        const parse::VarListNode* varList = argList->varList.get();
        while (varList) {
            assert(varList->nodeType == parse::NodeType::kVarList);
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                assert(varDef->nodeType == parse::NodeType::kVarDef);
                auto name = m_lexer->tokens()[varDef->tokenIndex].hash;
                frame->argumentOrder = library::ArrayedCollection::_ArrayAdd(context, frame->argumentOrder, name);
                Slot initialValue;
                if (varDef->initialValue) {
                    if (varDef->initialValue->nodeType == parse::NodeType::kLiteral) {
                        const auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                        initialValue = literal->value;
                    } else {
                        // TODO: add to list for if-block processing
                    }
                }
                frame->argumentDefaults = library::ArrayedCollection::_ArrayAdd(context, frame->argumentDefaults,
                        initialValue);
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        argList = reinterpret_cast<const parse::ArgListNode*>(argList->next.get());
    }

    // Variable definitions are allowed inline in Hadron, so we process variable definitions just like expression
    // sequences in the main body.
    if (blockNode->variables) {
        m_block->finalValue = buildFinalValue(context, blockNode->variables.get());
    }

    // ** TODO: insert any non-literal value initalizations here as ifNil blocks.

    if (blockNode->body) {
        m_block->finalValue = buildFinalValue(context, blockNode->body.get());
    }

    // If this is the root Frame we insert a return value, if not already inserted.
    if (frame->parent == nullptr) {
        findOrInsertLocal(std::make_unique<hir::StoreReturnHIR>(m_block->finalValue));
    }

    return frame;
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
            nodeValue.first = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot()));
            nodeValue.second = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot(Type::kNil)));
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
        findOrInsertLocal(std::make_unique<hir::StoreReturnHIR>(nodeValue));
    } break;

    case parse::NodeType::kList: {
        assert(false);  // TODO
    } break;

    case parse::NodeType::kDictionary: {
        assert(false);
    } break;

    // This only works for inline blocks <- when do these happen? Are they blockLiterals?
    case parse::NodeType::kBlock: {
        assert(false); // TODO: figure out when these happen and what's going on here.
        const auto blockNode = reinterpret_cast<const parse::BlockNode*>(node);
        // Preserve current frame and block, for wiring into the new frame and block. Encountering a block always
        // terminates the current block.
        Frame* parentFrame = m_frame;
        Block* lastBlock = m_block;

        // Recursively build the subframe.
        auto subFrame = buildSubframe(context, blockNode);

        // Wire the entry block in the new frame as a successor in the block graph.
        Block* frameEntryBlock = subFrame->blocks.front().get();
        lastBlock->successors.emplace_back(frameEntryBlock);
        frameEntryBlock->predecessors.emplace_back(lastBlock);

        // We need a new block in the parent frame as a sucessor to the exit block from the sub frame.
        assert(m_block == subFrame->blocks.back().get());
        auto parentBlock = std::make_unique<Block>(parentFrame, m_blockSerial);
        ++m_blockSerial;
        Block* frameExitBlock = m_block;
        parentBlock->predecessors.emplace_back(frameExitBlock);
        frameExitBlock->successors.emplace_back(parentBlock.get());
        m_block = parentBlock.get();
        parentFrame->blocks.emplace_back(std::move(parentBlock));
        parentFrame->subFrames.emplace_back(std::move(subFrame));
        m_frame = parentFrame;
    } break;

    case parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const parse::LiteralNode*>(node);
        nodeValue.first = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(literal->value));
        nodeValue.second = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot(literal->type)));
    } break;

    case parse::NodeType::kName: {
        const auto nameNode = reinterpret_cast<const parse::NameNode*>(node);
        auto name = m_lexer->tokens()[nameNode->tokenIndex].hash;
        nodeValue = findName(name); // <-- will need to update revisions in local block
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
        Frame* parentFrame = m_frame;
        Block* ifBlock = m_block;

        // Build the true condition block.
        assert(ifNode->trueBlock);
        auto trueFrameOwning = buildSubframe(context, ifNode->trueBlock.get());
        Frame* trueFrame = trueFrameOwning.get();
        parentFrame->subFrames.emplace_back(std::move(trueFrameOwning));
        branch->blockNumber = trueFrame->blocks.front()->number;
        ifBlock->successors.emplace_back(trueFrame->blocks.front().get());
        trueFrame->blocks.front()->predecessors.emplace_back(ifBlock);

        // Build the else condition block if present, if not build a default nil valued one. The BranchIfZero will
        // target branching here.
        Frame* falseFrame = nullptr;
        if (ifNode->falseBlock) {
            auto falseFrameOwning = buildSubframe(context, ifNode->falseBlock.get());
            falseFrame = falseFrameOwning.get();
            parentFrame->subFrames.emplace_back(std::move(falseFrameOwning));
        } else {
            auto falseFrameOwning = std::make_unique<Frame>();
            falseFrame = falseFrameOwning.get();
            parentFrame->subFrames.emplace_back(std::move(falseFrameOwning));
            falseFrame->parent = parentFrame;
            auto falseBlockOwning = std::make_unique<Block>(falseFrame, m_blockSerial);
            ++m_blockSerial;
            Block* falseBlock = falseBlockOwning.get();
            falseFrame->blocks.emplace_back(std::move(falseBlockOwning));
            falseBlock->finalValue.first = insert(std::make_unique<hir::ConstantHIR>(Slot()), falseBlock);
            falseBlock->finalValue.second = insert(std::make_unique<hir::ConstantHIR>(Slot(Type::kNil)), falseBlock);
        }
        condBranch->blockNumber = falseFrame->blocks.front()->number;
        ifBlock->successors.emplace_back(falseFrame->blocks.front().get());
        falseFrame->blocks.front()->predecessors.emplace_back(ifBlock);

        // Create a new block in the parent frame for code after the if statement.
        auto continueBlock = std::make_unique<Block>(parentFrame, m_blockSerial);
        ++m_blockSerial;
        m_block = continueBlock.get();
        parentFrame->blocks.emplace_back(std::move(continueBlock));
        m_frame = parentFrame;

        // Add phis with the final values of both the false and true values here, this is also the value of the
        // if statement.
        auto valuePhi = std::make_unique<hir::PhiHIR>();
        valuePhi->addInput(trueFrame->blocks.back()->finalValue.first);
        valuePhi->addInput(falseFrame->blocks.back()->finalValue.first);
        nodeValue.first = valuePhi->proposeValue(m_valueSerial);
        m_block->localValues.emplace(std::make_pair(nodeValue.first, nodeValue.first));
        ++m_valueSerial;
        m_block->phis.emplace_back(std::move(valuePhi));
        auto typePhi = std::make_unique<hir::PhiHIR>();
        typePhi->addInput(trueFrame->blocks.back()->finalValue.second);
        typePhi->addInput(falseFrame->blocks.back()->finalValue.second);
        nodeValue.second = typePhi->proposeValue(m_valueSerial);
        m_block->localValues.emplace(std::make_pair(nodeValue.second, nodeValue.second));
        ++m_valueSerial;
        m_block->phis.emplace_back(std::move(typePhi));

        // Wire trueFrame exit block to the continue block here.
        auto trueBranch = std::make_unique<hir::BranchHIR>();
        trueBranch->blockNumber = m_block->number;
        insert(std::move(trueBranch), trueFrame->blocks.back().get());
        trueFrame->blocks.back()->successors.emplace_back(m_block);
        m_block->predecessors.emplace_back(trueFrame->blocks.back().get());

        // Wire falseFrame exit block to the continue block here.
        assert(falseFrame);
        auto falseBranch = std::make_unique<hir::BranchHIR>();
        falseBranch->blockNumber = m_block->number;
        insert(std::move(falseBranch), falseFrame->blocks.back().get());
        falseFrame->blocks.back()->successors.emplace_back(m_block);
        m_block->predecessors.emplace_back(falseFrame->blocks.back().get());
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

    // Going to need the symbol type handy for insertion as the selector and any keyword arguments.
    auto symbolType = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot(Type::kType)));
    argumentValues.emplace_back(std::make_pair(findOrInsertLocal(std::make_unique<hir::ConstantHIR>(selector)),
            symbolType));

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
                    findOrInsertLocal(std::make_unique<hir::ConstantHIR>(keyName)), symbolType));
        keywordArgumentValues.emplace_back(buildFinalValue(context, keywordArguments->value.get()));
        keywordArguments = reinterpret_cast<const parse::KeyValueNode*>(keywordArguments->next.get());
    }

    // Now setup the stack for the dispatch.
    insertLocal(std::make_unique<hir::DispatchSetupStackHIR>(argumentValues.size(), keywordArgumentValues.size() / 2));
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

Value BlockBuilder::findOrInsertLocal(std::unique_ptr<hir::HIR> hir) {
    // Walk through block looking for equivalent exising HIR.
    for (const auto hirPair : m_block->values) {
        if (hirPair.second->isEquivalent(hir.get())) {
            return hirPair.first;
        }
    }
    return insertLocal(std::move(hir));
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
    // First we search for names already defined in the local block.
    auto rev = m_block->revisions.find(name);
    if (rev != m_block->revisions.end()) {
        return rev->second;
    }

    // Next we look up through block predecessors for local definitions which will either be previously loaded
    // argument values *or* local variables. However, we *only* look inside blocks that are in this frame or
    // parent frames.
    Frame* frame = m_frame;
    std::vector<const Frame*> lineage(1, frame);
    frame = frame->parent;
    while (frame != nullptr) {
        lineage.emplace_back(frame);
        frame = frame->parent;
    }
    // There's some work to do to determine policy around inline frames vs. frames that get their own stack frame.
    // Perhaps frame is the wrong term, maybe scope is better. BlockBuilder only supports top-level arguments,
    // generally. There may be some additional work to do to inline blocks in loops, which typically do take arguments,
    // but I think inline blocks arguments are just converted to local variables *in the containing frame scope*.
    // Encountering a block starts a whole new compilation pipeline that results in a library::FunctionDef object, that
    // receives dispatches like any other object. Lexical closure will require some additional work here too. Perhaps we
    // could treat the containing variables like variable instances in an Object?
    // For now: only top-level frame gets to define *arguments*, and we search those last after searching up the blocks
    // for variables.
    // For loading arguments, because we're always in the same stack frame, we insert (or re-use) loads into the top
    // frame, then propagate that value via findValue() all the way back to the block that needs it, allowing it to
    // propagate through various phis as needed. This takes care of blocks in a lower scope may alter argument values
    // (like the if(argName.isNil, { argName = foo; }) sort of constructions. This will mean unused arguments never get
    // a load inserted.
    // Late loading optimization pass - to ease register pressure, we could later do an optimization pass where, after
    // dead code deletion we could move statements that produce values down until right before they are first read.


    return std::make_pair(Value(), Value());
}

Value BlockBuilder::findValue(Value v) {
    std::unordered_map<int, Value> blockValues;
    return findValuePredecessor(v, m_block, blockValues);
}

Value BlockBuilder::findValuePredecessor(Value v, Block* block, std::unordered_map<int, Value>& blockValues) {
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