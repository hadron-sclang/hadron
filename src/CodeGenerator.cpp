#include "CodeGenerator.hpp"

#include "Block.hpp"
#include "HIR.hpp"
#include "Keywords.hpp"
#include "Parser.hpp"
#include "Value.hpp"

#include "fmt/core.h"
#include "spdlog/spdlog.h"

#include <vector>

namespace hadron {

std::unique_ptr<Block> CodeGenerator::buildBlock(const parse::BlockNode* blockNode) {
    auto block = std::make_unique<Block>(m_blockSerial);
    m_blockSerial++;

    // TODO:arguments
    if (blockNode->variables) {
        buildBlockHIR(blockNode->variables.get(), block.get());
    }
    if (blockNode->body) {
        buildBlockHIR(blockNode->body.get(), block.get());
    }
    return block;
}

// Processes both variable declaration and expr IRs. 
void CodeGenerator::buildBlockHIR(const parse::Node* node, Block* block) {
    switch (node->nodeType) {
    case hadron::parse::NodeType::kEmpty:
        spdlog::error("CodeGenerator encountered empty parse node.");
        return;

    // might make sense to break these out to specialized methods like in the parse tree, possibly even public methods
    // for ease of testability
    case hadron::parse::NodeType::kVarDef: {
        const auto varDef = reinterpret_cast<const parse::VarDefNode*>(node);
        block->values.emplace(std::make_pair(varDef->nameHash, Value(Type::kInteger, varDef->varName)));
        // Initial value computation can be a whole expression sequence which we would add to the block. For this first
        // pass we look only for literal ints, and if we find none, issue a warning and choose zero.
        if (varDef->initialValue) {
            if (varDef->initialValue->nodeType == parse::NodeType::kLiteral) {
                const auto literal = reinterpret_cast<const parse::LiteralNode*>(varDef->initialValue.get());
                if (literal->value.type() == Type::kInteger) {
                    block->instructions.emplace_back(HIR{Opcode::kAssignI32,
                        {ValueRef{block, varDef->nameHash, varDef->varName}, literal->value}});
                } else {
                    spdlog::warn("Non-integer initial value for var {}", std::string(varDef->varName.data(),
                        varDef->varName.size()));
                }
            } else {
                spdlog::warn("Non-literal inital value for var {}", std::string(varDef->varName.data(),
                    varDef->varName.size()));
            }
        } else {
            spdlog::warn("Failed to find initial value for var {}", std::string(varDef->varName.data(),
                varDef->varName.size()));
        }
    } break;


    case hadron::parse::NodeType::kVarList: {
        const auto varList = reinterpret_cast<const parse::VarListNode*>(node);
        if (varList->definitions) {
            buildBlockHIR(varList->definitions.get(), block);
        } else {
            spdlog::warn("VarList with absent definitions - parse error.");
        }
    } break;

/*
    kArgList, leave out
    kMethod, leave out
    kClassExt, leave out
    kClass, leave out
    kReturn,
    kDynList,
*/
    case hadron::parse::NodeType::kBlock: {
        // Kind of a weird one to run across in this context
        spdlog::error("got Block while parsing block");
    } break;

/*
    kValue,
    kLiteral,

    case hadron::parse::NodeType::kName: {

    } break;
*/

    case hadron::parse::kExprSeq: {
        // Appending to existing block.
        const auto exprSeq = reinterpret_cast<const parse::ExprSeqNode*>(node);
        if (exprSeq->expr) {
            buildBlockHIR(exprSeq->expr.get(), block);
        }
    } break;
/*
    kAssign,
    kSetter,
    kKeyValue,
*/
    case hadron::parse::kCall: {
        const auto call = reinterpret_cast<const parse::CallNode*>(node);
        if (call->selector == kWhileHash) {
            // bingo
        }
    } break;

    case hadron::parse::kBinopCall: {

    } break;

/*
    kPerformList,
    kNumericSeries
*/

    default:
        spdlog::warn("buildHIR got unsupported ParseTree node");
        break;
    }

    if (node->next) {
        buildBlockHIR(node->next.get(), block);
    }
}

} // namespace hadron

