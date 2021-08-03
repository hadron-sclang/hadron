#include "hadron/SSABuilder.hpp"

#include "hadron/ErrorReporter.hpp"
#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "hadron/Slot.hpp"


namespace hadron {

SSABuilder::SSABuilder(Lexer* lexer, std::shared_ptr<ErrorReporter> errorReporter):
    m_lexer(lexer), m_errorReporter(errorReporter), m_frame(nullptr), m_block(nullptr) { }

SSABuilder::~SSABuilder() { }

std::unique_ptr<Frame> SSABuilder::build(const Lexer* lexer, const parse::BlockNode* blockNode) {
    assert(blockNode);
    auto frame = std::make_unique<Frame>();
    // Make an entry block and add to frame.
    m_frame = frame.get();
    frame->blocks.emplace_back(std::make_unique<Block>(m_frame, 0));
    m_block = frame->blocks.begin()->get();

    // Build argument name list and default values.
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
                auto name = lexer->tokens()[varDef->tokenIndex].hash;
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
                m_block->nameRevisions[name] = findOrInsert(std::make_unique<hir::LoadArgumentHIR>(argIndex, true));
                m_block->typeRevisions[name] = findOrInsert(std::make_unique<hir::LoadArgumentHIR>(argIndex, false));
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
        finalValue = buildValue(blockNode->variables.get());
    }
    if (blockNode->body) {
        finalValue = buildValue(blockNode->body.get());
    }

    // Add a *save to return* HIR with final value and type, which will only be added if not redundant with an already
    // specified idential return value.
    findOrInsert(std::make_unique<hir::StoreReturnHIR>(finalValue));
    return frame;
}

std::pair<int32_t, int32_t> SSABuilder::buildValue(const parse::Node* node) {
    std::pair<int32_t, int32_t> nodeValue = std::make_pair(-1, -1);
    switch(node->nodeType) {
    case parse::NodeType::kEmpty:
        assert(false); // kEmpty is invalid node type.
        return std::make_pair(-1, -1);
        break;

    case parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const parse::VarDefNode*>(node);
        auto nameToken = m_lexer->tokens()[varDef->tokenIndex];
        // TODO: error reporting for variable redefinition
        if (varDef->initialValue) {
            nodeValue = buildValue(varDef->initialValue.get());
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
            nodeValue = buildValue(varList->definitions.get());
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
        nodeValue = buildValue(returnNode->valueExpr.get());
        // We assign a unique value to the result of calling StoreReturn in the off chance that it has already been
        // called with identical values, in the same block. StoreReturn will be called automatically with the last
        // block value, so it is possible that this call makes the automatic store return redundant. But the return
        // value pair of this return is still the nodeValue of the expression sequence within returnNode.
        findOrInsert(std::make_unique<hir::StoreReturnHIR>(nodeValue));
    } break;

    case parse::NodeType::kDynList: {
        assert(false);  // TODO
    } break;

    case parse::NodeType::kBlock: {
        // This represents, in fact, the creation of a new *Frame* of scope for local variables. But very odd to
        // encounter in this kind of context.
    } break;

    case parse::NodeType::kLiteral: {
        const auto literal = reinterpret_cast<const parse::LiteralNode*>(node);
        nodeValue.first = findOrInsert(std::make_unique<hir::ConstantHIR>(literal->value));
        nodeValue.second = findOrInsert(std::make_unique<hir::ConstantHIR>(
            Slot(Type::kType, Slot::Value(literal->value.type))));
    } break;

    default: // DELETE ME
        break;
    }

    if (node->next) {
        nodeValue = buildValue(node->next.get());
    }

    return nodeValue;
}

int32_t SSABuilder::findOrInsert(std::unique_ptr<hir::HIR> hir) {
    if (!hir->isAlwaysUnique()) {
        // Walk through block looking for equivalent exising hir.
        for (const auto hirPair : m_block->values) {
            if (hirPair.second->isEquivalent(hir.get())) {
                return hirPair.first;
            }
        }
    }
    int32_t valueNumber = m_frame->valueCount;
    hir->valueNumber = valueNumber;
    ++m_frame->valueCount;
    m_block->values.emplace(std::make_pair(valueNumber, hir.get()));
    m_block->statements.emplace_back(std::move(hir));
    return valueNumber;
}

} // namespace hadron