#include "hadron/SSABuilder.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Slot.hpp"

#include "fmt/format.h"

#include <string>

namespace hadron {

SSABuilder::SSABuilder(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(lexer),
    m_errorReporter(errorReporter),
    m_frame(nullptr),
    m_block(nullptr),
    m_blockSerial(0),
    m_valueSerial(1) { }

SSABuilder::~SSABuilder() { }

std::unique_ptr<Frame> SSABuilder::buildFrame(const parse::BlockNode* blockNode) {
    auto frame = buildSubframe(blockNode);
    frame->numberOfBlocks = m_blockSerial;
    frame->numberOfValues = m_valueSerial;
    return frame;
}

std::unique_ptr<Frame> SSABuilder::buildSubframe(const parse::BlockNode* blockNode) {
    assert(blockNode);
    auto frame = std::make_unique<Frame>();
    frame->parent = m_frame;
    m_frame = frame.get();
    // Make an entry block and add to frame.
    frame->blocks.emplace_back(std::make_unique<Block>(m_frame, m_blockSerial));
    ++m_blockSerial;
    m_block = frame->blocks.begin()->get();

    // Build argument name list and default values.

    // Hack to always append *this* which actually has a known type, interesting.
    const parse::ArgListNode* argList = blockNode->arguments.get();
    int32_t argIndex = 0;
    while (argList) {
        assert(argList->nodeType == parse::NodeType::kArgList);
        const parse::VarListNode* varList = argList->varList.get();
        while (varList) {
            assert(varList->nodeType == parse::NodeType::kVarList);
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                assert(varDef->nodeType == parse::NodeType::kVarDef);
                auto name = m_lexer->tokens()[varDef->tokenIndex].hash;
                frame->argumentOrder.emplace_back(name);
                // TODO: for the ARG keyword the initial value can be *any* valid expr parse. For the | pair argument
                // delimiters valid syntax is any slotliteral, so basically anything that can be trivially packed
                // into a Slot. This seems a much more tractable way to describe initial values, so investigate
                // locking the parser down to only accept those for both argument styles. My suspcision is that
                // complex expressions using the `arg` syntax may parse but probably won't survive later compilation
                // stages in LSC anyway, so this may just represent making a de-facto LSC error into an explicit
                // parse error. Or, there's going to have to be an *if* block added to each variable in turn with
                // something like `if variable missing then do init expr` which just seems terrible.
                // For now we just do assignments with the LoadArgument opcode.
                m_block->revisions[name] = std::make_pair(
                    insertLocal(std::make_unique<hir::LoadArgumentHIR>(m_frame, argIndex)),
                    insertLocal(std::make_unique<hir::LoadArgumentTypeHIR>(m_frame, argIndex)));
                ++argIndex;
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        argList = reinterpret_cast<const parse::ArgListNode*>(argList->next.get());
    }

    // Variable definitions are allowed inline in Hadron, so we process variable definitions just like expression
    // sequences in the main body.
    if (blockNode->variables) {
        buildFinalValue(blockNode->variables.get());
    }
    if (blockNode->body) {
        buildFinalValue(blockNode->body.get());
    }

    return frame;
}

std::pair<Value, Value> SSABuilder::buildValue(const parse::Node* node) {
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
            nodeValue = buildFinalValue(varDef->initialValue.get());
            m_block->revisions[nameToken.hash] = nodeValue;
        } else {
            nodeValue.first = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot()));
            nodeValue.second = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(
                    Slot(Type::kType, Slot::Value(Type::kNil))));
            m_block->revisions[nameToken.hash] = nodeValue;
        }
    } break;

    case parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const parse::VarListNode*>(node);
        if (varList->definitions) {
            nodeValue = buildFinalValue(varList->definitions.get());
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
        nodeValue = buildFinalValue(returnNode->valueExpr.get());
        findOrInsertLocal(std::make_unique<hir::StoreReturnHIR>(m_frame, nodeValue));
    } break;

    case parse::NodeType::kDynList: {
        assert(false);  // TODO
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
        auto subFrame = buildSubframe(blockNode);

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
        nodeValue.second = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(
            Slot(Type::kType, Slot::Value(literal->value.type))));
    } break;

    case parse::NodeType::kName: {
        const auto nameNode = reinterpret_cast<const parse::NameNode*>(node);
        auto name = m_lexer->tokens()[nameNode->tokenIndex].hash;
        // TODO: keywords like `this` live here.
        nodeValue = findName(name); // <-- this needs to update names in the local block!
    } break;

    case parse::NodeType::kExprSeq: {
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(node);
        assert(exprSeq->expr);
        nodeValue = buildFinalValue(exprSeq->expr.get());
    } break;

    case parse::NodeType::kAssign: {
        const auto assign = reinterpret_cast<const parse::AssignNode*>(node);
        assert(assign->name);
        assert(assign->value);
        nodeValue = buildFinalValue(assign->value.get());
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
        nodeValue = buildDispatch(setter->target.get(), selector, setter->value.get(), nullptr);
    } break;

    case parse::NodeType::kKeyValue:
        assert(false);  // Top-level keyvalue pair is a syntax error.
        break;

    case parse::NodeType::kCall: {
        const auto call = reinterpret_cast<const parse::CallNode*>(node);
        auto selector = m_lexer->tokens()[call->tokenIndex].hash;
        nodeValue = buildDispatch(call->target.get(), selector, call->arguments.get(), call->keywordArguments.get());
    } break;

    case parse::NodeType::kBinopCall: {
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(node);
        auto selector = m_lexer->tokens()[binop->tokenIndex].hash;
        nodeValue = buildDispatch(binop->leftHand.get(), selector, binop->rightHand.get(), nullptr);
    } break;

    case parse::NodeType::kPerformList: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kNumericSeries: {
        assert(false); // TODO
    } break;

    case parse::NodeType::kIf: {
        const auto ifNode = reinterpret_cast<const parse::IfNode*>(node);
        auto condition = buildFinalValue(ifNode->condition.get());
        auto condBranchOwning = std::make_unique<hir::BranchIfZeroHIR>(condition);
        // Insert the if HIR but keep a copy of the pointer around to connect it to the continuation or else blocks
        hir::BranchIfZeroHIR* condBranch = condBranchOwning.get();
        nodeValue.first = insertLocal(std::move(condBranchOwning));
        nodeValue.second = insertLocal(std::make_unique<hir::ResolveTypeHIR>(nodeValue.first));
        auto branchOwning = std::make_unique<hir::BranchHIR>();
        hir::BranchHIR* branch = branchOwning.get();
        insertLocal(std::move(branchOwning));

        // Preserve the current block and frame for insertLocalion of the new subframes as children.
        Frame* parentFrame = m_frame;
        Block* ifBlock = m_block;

        // Build the true condition block. We unconditionally branch to this with the expectation that it will be
        // serialized after the if block, meaning we can delete the branch.
        assert(ifNode->trueBlock);
        auto trueFrameOwning = buildSubframe(ifNode->trueBlock.get());
        Frame* trueFrame = trueFrameOwning.get();
        parentFrame->subFrames.emplace_back(std::move(trueFrameOwning));
        branch->blockNumber = trueFrame->blocks.front()->number;
        ifBlock->successors.emplace_back(trueFrame->blocks.front().get());
        trueFrame->blocks.front()->predecessors.emplace_back(ifBlock);

        // Build the else condition block if present. The BranchIfZero will target branching here.
        Frame* falseFrame = nullptr;
        if (ifNode->falseBlock) {
            auto falseFrameOwning = buildSubframe(ifNode->falseBlock.get());
            falseFrame = falseFrameOwning.get();
            parentFrame->subFrames.emplace_back(std::move(falseFrameOwning));
            condBranch->blockNumber = falseFrame->blocks.front()->number;
            ifBlock->successors.emplace_back(falseFrame->blocks.front().get());
            falseFrame->blocks.front()->predecessors.emplace_back(ifBlock);
        }

        // Create a new block in the parent frame for code after the if statement.
        auto continueBlock = std::make_unique<Block>(parentFrame, m_blockSerial);
        ++m_blockSerial;
        m_block = continueBlock.get();
        parentFrame->blocks.emplace_back(std::move(continueBlock));
        m_frame = parentFrame;

        // Wire trueFrame exit block to the continue block here.
        trueFrame->blocks.back()->successors.emplace_back(m_block);
        m_block->predecessors.emplace_back(trueFrame->blocks.back().get());

        // Either wire the else condition to the continue block, or wire the if false condition directly here if no
        // else condition specified.
        if (falseFrame) {
            auto falseBranch = std::make_unique<hir::BranchHIR>();
            falseBranch->blockNumber = m_block->number;
            insert(std::move(falseBranch), falseFrame->blocks.back().get());
            falseFrame->blocks.back()->successors.emplace_back(m_block);
            m_block->predecessors.emplace_back(falseFrame->blocks.back().get());
        } else {
            condBranch->blockNumber = m_block->number;
            ifBlock->successors.emplace_back(m_block);
            m_block->predecessors.emplace_back(ifBlock);
        }
    } break;
    }

    nodeValue.first = findValue(nodeValue.first);
    nodeValue.second = findValue(nodeValue.second);
    return nodeValue;
}

std::pair<Value, Value> SSABuilder::buildFinalValue(const parse::Node* node) {
    std::pair<Value, Value> finalValue;
    while (node) {
        finalValue = buildValue(node);
        node = node->next.get();
    }
    return finalValue;
}

std::pair<Value, Value> SSABuilder::buildDispatch(const parse::Node* target, Hash selector,
        const parse::Node* arguments, const parse::KeyValueNode* keywordArguments) {
    auto dispatch = std::make_unique<hir::DispatchCallHIR>();
    auto targetValue = buildFinalValue(target);
    // Build argument list starting with target argument as `this`.
    dispatch->addArgument(targetValue);
    // Going to need the symbol type handy for insertion as the selector and any keyword arguments.
    auto symbolType = findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot(Type::kType,
        Slot::Value(Type::kSymbol))));
    // Next is selector added as a symbol constant.
    dispatch->addArgument(std::make_pair(findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot(Type::kSymbol,
        Slot::Value(selector)))), symbolType));

    // Now append any additional arguments.
    while (arguments) {
        dispatch->addArgument(buildValue(arguments));
        arguments = arguments->next.get();
    }

    // Now append any keyword arguments.
    while (keywordArguments) {
        assert(keywordArguments->nodeType == parse::NodeType::kKeyValue);
        auto keyName = m_lexer->tokens()[keywordArguments->tokenIndex].hash;
        auto key = std::make_pair(findOrInsertLocal(std::make_unique<hir::ConstantHIR>(Slot(Type::kSymbol,
            Slot::Value(keyName)))), symbolType);
        dispatch->addKeywordArgument(key, buildFinalValue(keywordArguments->value.get()));
        keywordArguments = reinterpret_cast<const parse::KeyValueNode*>(keywordArguments->next.get());
    }

    // Add the dispatch call and get the updated value number for the target, to record the mutation of the target.
    Value updatedTarget = insertLocal(std::move(dispatch));

    // Do a reverse search on local names to update their values to new target as needed. Type is assumed to be
    // invariant for the target. If this was an anonymous target nothing will be updated, but if there were
    // local names that were tracking the target value they must be updated to reflect any modifications
    // by side effects of the dispatch.
    for (auto iter : m_block->revisions) {
        if (iter.second == targetValue) {
            iter.second.first = updatedTarget;
        }
    }

    Value returnValue = insertLocal(std::make_unique<hir::DispatchLoadReturnHIR>());
    Value returnValueType = insertLocal(std::make_unique<hir::DispatchLoadReturnTypeHIR>());
    insertLocal(std::make_unique<hir::DispatchCleanupHIR>());
    return std::make_pair(returnValue, returnValueType);
}

Value SSABuilder::findOrInsertLocal(std::unique_ptr<hir::HIR> hir) {
    // Walk through block looking for equivalent exising HIR.
    for (const auto hirPair : m_block->values) {
        if (hirPair.second->isEquivalent(hir.get())) {
            return hirPair.first;
        }
    }
    return insertLocal(std::move(hir));
}

Value SSABuilder::insertLocal(std::unique_ptr<hir::HIR> hir) {
    return insert(std::move(hir), m_block);
}

Value SSABuilder::insert(std::unique_ptr<hir::HIR> hir, Block* block) {
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

std::pair<Value, Value> SSABuilder::findName(Hash name) {
    // TODO: out-of-block searches.
    auto rev = m_block->revisions.find(name);
    if (rev == m_block->revisions.end()) {
        return std::make_pair(Value(), Value());
    }
    return rev->second;
}

Value SSABuilder::findValue(Value v) {
    std::unordered_map<int, Value> blockValues;
    return findValuePredecessor(v, m_block, blockValues);
}

Value SSABuilder::findValuePredecessor(Value v, Block* block, std::unordered_map<int, Value>& blockValues) {
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