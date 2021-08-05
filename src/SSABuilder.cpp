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
                m_block->nameRevisions[name] = insert(std::make_unique<hir::LoadArgumentHIR>(argIndex, true));
                // TODO: the type of any argument should always be kAny, to inform the type deduction system that
                // we cannot assume the type of any argument. But we may need to execute the instruction at runtime to
                // load the type. Perhaps there's a pass while cleaning up type phis where we can interpret this HIR as
                // essentially a constant.
                m_block->typeRevisions[name] = insert(std::make_unique<hir::LoadArgumentHIR>(argIndex, false));
                ++argIndex;
                varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
            }
            varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
        }
        argList = reinterpret_cast<const parse::ArgListNode*>(argList->next.get());
    }

    // Variable definitions are allowed inline in Hadron, so we process variable definitions just like expression
    // sequences in the main body.
    std::pair<int32_t, int32_t> finalValue = std::make_pair(-1, -1);
    if (blockNode->variables) {
        finalValue = buildFinalValue(blockNode->variables.get());
    }
    if (blockNode->body) {
        finalValue = buildFinalValue(blockNode->body.get());
    }
    // TODO: what to do with finalValue?
    return frame;
}

std::pair<int32_t, int32_t> SSABuilder::buildValue(const parse::Node* node) {
    std::pair<int32_t, int32_t> nodeValue = std::make_pair(-1, -1);
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
            m_block->nameRevisions[nameToken.hash] = nodeValue.first;
            m_block->typeRevisions[nameToken.hash] = nodeValue.second;
        } else {
            auto initialValue = std::make_unique<hir::ConstantHIR>(Slot());
            nodeValue.first = findOrInsert(std::move(initialValue));
            m_block->nameRevisions[nameToken.hash] = nodeValue.first;
            auto initialType = std::make_unique<hir::ConstantHIR>(Slot(Type::kType, Slot::Value(Type::kNil)));
            nodeValue.second = findOrInsert(std::move(initialType));
            m_block->typeRevisions[nameToken.hash] = nodeValue.second;
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
        findOrInsert(std::make_unique<hir::StoreReturnHIR>(nodeValue));
    } break;

    case parse::NodeType::kDynList: {
        assert(false);  // TODO
    } break;

    // This only works for inline blocks <- when do these happen?
    case parse::NodeType::kBlock: {
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
        nodeValue.first = findOrInsert(std::make_unique<hir::ConstantHIR>(literal->value));
        nodeValue.second = findOrInsert(std::make_unique<hir::ConstantHIR>(
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
        m_block->nameRevisions[name] = nodeValue.first;
        m_block->typeRevisions[name] = nodeValue.second;
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
    }

    return nodeValue;
}

std::pair<int32_t, int32_t> SSABuilder::buildFinalValue(const parse::Node* node) {
    auto finalValue = std::make_pair(-1, -1);
    while (node) {
        finalValue = buildValue(node);
        node = node->next.get();
    }

    return finalValue;
}

std::pair<int32_t, int32_t> SSABuilder::buildDispatch(const parse::Node* target, Hash selector,
    const parse::Node* arguments, const parse::KeyValueNode* keywordArguments) {
    auto dispatch = std::make_unique<hir::DispatchCallHIR>();
    auto targetValues = buildFinalValue(target);
    // Build argument list starting with target argument as `this`.
    dispatch->addArgument(targetValues);
    // Next is selector added as a symbol.
    dispatch->addArgument(std::make_pair(
        findOrInsert(std::make_unique<hir::ConstantHIR>(Slot(Type::kSymbol, Slot::Value(selector)))),
        findOrInsert(std::make_unique<hir::ConstantHIR>(Slot(Type::kType, Slot::Value(Type::kSymbol))))));
    // Now append any additional arguments.
    while (arguments) {
        dispatch->addArgument(buildValue(arguments));
        arguments = arguments->next.get();
    }
    // Now append any keyword arguments.
    while (keywordArguments) {
        auto keyName = m_lexer->tokens()[keywordArguments->tokenIndex].hash;
        dispatch->addKeywordArgument(std::make_pair(
            findOrInsert(std::make_unique<hir::ConstantHIR>(Slot(Type::kSymbol, Slot::Value(keyName)))),
            findOrInsert(std::make_unique<hir::ConstantHIR>(Slot(Type::kType, Slot::Value(Type::kSymbol))))),
            buildFinalValue(keywordArguments->value.get()));
        keywordArguments = reinterpret_cast<const parse::KeyValueNode*>(keywordArguments->next.get());
    }

    // Add the dispatch call and get the updated value number for the target, to record the mutation.
    int32_t updatedTarget = insert(std::move(dispatch));
    // Do a reverse search on local names to update their values to new target as needed. Type is assumed to be
    // invariant for the target. If this was an anonymous target nothing will be updated, but if there were
    // local names that were tracking the target value they must be updated to reflect any modifications
    // by the setter call.
    for (auto iter : m_block->nameRevisions) {
        if (iter.second == targetValues.first) {
            iter.second = updatedTarget;
        }
    }

    std::pair<int32_t, int32_t> returnValue;
    returnValue.first = insert(std::make_unique<hir::DispatchLoadReturnHIR>(true));
    returnValue.second = insert(std::make_unique<hir::DispatchLoadReturnHIR>(false));
    insert(std::make_unique<hir::DispatchCleanupHIR>());
    return returnValue;
}

int32_t SSABuilder::findOrInsert(std::unique_ptr<hir::HIR> hir) {
    // Walk through block looking for equivalent exising HIR.
    for (const auto hirPair : m_block->values) {
        if (hirPair.second->isEquivalent(hir.get())) {
            return hirPair.first;
        }
    }
    return insert(std::move(hir));
}

int32_t SSABuilder::insert(std::unique_ptr<hir::HIR> hir) {
    int32_t valueNumber = m_valueSerial;
    hir->valueNumber = valueNumber;
    ++m_valueSerial;
    m_block->values.emplace(std::make_pair(valueNumber, hir.get()));
    m_block->statements.emplace_back(std::move(hir));
    return valueNumber;
}

} // namespace hadron