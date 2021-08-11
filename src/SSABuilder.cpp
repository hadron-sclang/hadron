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
    m_valueSerial(0) { }

SSABuilder::~SSABuilder() { }

std::unique_ptr<Frame> SSABuilder::buildFrame(const parse::BlockNode* blockNode) {
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
                m_block->revisions[name] = insert(std::make_unique<hir::LoadArgumentHIR>(m_frame, argIndex));
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

Value SSABuilder::buildValue(const parse::Node* node) {
    Value nodeValue;
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
            auto initialValue = std::make_unique<hir::ConstantHIR>(Slot());
            nodeValue = findOrInsert(std::move(initialValue));
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
        findOrInsert(std::make_unique<hir::StoreReturnHIR>(m_frame, nodeValue));
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
        auto subFrame = buildFrame(blockNode);

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
        nodeValue = findOrInsert(std::make_unique<hir::ConstantHIR>(literal->value));
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
        auto ifHIROwning = std::make_unique<hir::IfHIR>(condition);
        // Insert the if HIR but keep a copy of the pointer around to connect it to the true and false blocks.
        hir::IfHIR* ifHIR = ifHIROwning.get();
        insert(std::move(ifHIROwning));
        // Preserve the current block and frame for insertion of the new subframes as children.
        Frame* parentFrame = m_frame;
        Block* ifBlock = m_block;

        // Build the true condition block.
        assert(ifNode->trueBlock);
        auto trueFrameOwning = buildFrame(ifNode->trueBlock.get());
        Frame* trueFrame = trueFrameOwning.get();
        parentFrame->subFrames.emplace_back(std::move(trueFrameOwning));
        ifHIR->trueBlock = trueFrame->blocks.front()->number;
        ifBlock->successors.emplace_back(trueFrame->blocks.front().get());
        trueFrame->blocks.front()->predecessors.emplace_back(ifBlock);

        // Build the else condition block if present.
        Frame* falseFrame = nullptr;
        if (ifNode->falseBlock) {
            auto falseFrameOwning = buildFrame(ifNode->falseBlock.get());
            falseFrame = falseFrameOwning.get();
            parentFrame->subFrames.emplace_back(std::move(falseFrameOwning));
            ifHIR->falseBlock = falseFrame->blocks.front()->number;
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
            falseFrame->blocks.back()->successors.emplace_back(m_block);
            m_block->predecessors.emplace_back(falseFrame->blocks.back().get());
        } else {
            ifHIR->falseBlock = m_block->number;
            ifBlock->successors.emplace_back(m_block);
            m_block->predecessors.emplace_back(ifBlock);
        }
    } break;
    }

    return nodeValue;
}

Value SSABuilder::buildFinalValue(const parse::Node* node) {
    Value finalValue;
    while (node) {
        finalValue = buildValue(node);
        node = node->next.get();
    }

    return finalValue;
}

Value SSABuilder::buildDispatch(const parse::Node* target, Hash selector, const parse::Node* arguments,
        const parse::KeyValueNode* keywordArguments) {
    auto dispatch = std::make_unique<hir::DispatchCallHIR>();
    auto targetValue = buildFinalValue(target);
    // Build argument list starting with target argument as `this`.
    dispatch->addArgument(targetValue);
    // Next is selector added as a symbol constant.
    dispatch->addArgument(findOrInsert(std::make_unique<hir::ConstantHIR>(Slot(Type::kSymbol, Slot::Value(selector)))));
    // Now append any additional arguments.
    while (arguments) {
        dispatch->addArgument(buildValue(arguments));
        arguments = arguments->next.get();
    }
    // Now append any keyword arguments.
    while (keywordArguments) {
        assert(keywordArguments->nodeType == parse::NodeType::kKeyValue);
        auto keyName = m_lexer->tokens()[keywordArguments->tokenIndex].hash;
        dispatch->addKeywordArgument(
            findOrInsert(std::make_unique<hir::ConstantHIR>(Slot(Type::kSymbol, Slot::Value(keyName)))),
            buildFinalValue(keywordArguments->value.get()));
        keywordArguments = reinterpret_cast<const parse::KeyValueNode*>(keywordArguments->next.get());
    }

    // Add the dispatch call and get the updated value number for the target, to record the mutation of the target.
    Value updatedTarget = insert(std::move(dispatch));
    // Do a reverse search on local names to update their values to new target as needed. Type is assumed to be
    // invariant for the target. If this was an anonymous target nothing will be updated, but if there were
    // local names that were tracking the target value they must be updated to reflect any modifications
    // by the setter call.
    for (auto iter : m_block->revisions) {
        if (iter.second == targetValue) {
            iter.second = updatedTarget;
        }
    }

    Value returnValue = insert(std::make_unique<hir::DispatchLoadReturnHIR>());
    insert(std::make_unique<hir::DispatchCleanupHIR>());
    return returnValue;
}

Value SSABuilder::findOrInsert(std::unique_ptr<hir::HIR> hir) {
    // Walk through block looking for equivalent exising HIR.
    for (const auto hirPair : m_block->values) {
        if (hirPair.second->isEquivalent(hir.get())) {
            return hirPair.first;
        }
    }
    return insert(std::move(hir));
}

Value SSABuilder::insert(std::unique_ptr<hir::HIR> hir) {
    auto valueNumber = m_valueSerial;
    auto value = hir->proposeValue(valueNumber);
    // We don't bump the value serial for invalid values (meaning read-only operations)
    if (value.isValid()) {
        ++m_valueSerial;
        m_block->values.emplace(std::make_pair(value, hir.get()));
    }
    m_block->statements.emplace_back(std::move(hir));
    return value;
}

Value SSABuilder::findName(Hash name) {
    // TODO: out-of-block searches.
    auto rev = m_block->revisions.find(name);
    if (rev == m_block->revisions.end()) {
        return Value();
    }
    return rev->second;
}

} // namespace hadron