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

std::unique_ptr<Block> CodeGenerator::buildBlock(const parse::BlockNode* blockNode, Block* parent) {
    auto block = std::make_unique<Block>(m_blockSerial);
    m_blockSerial++;
    block->scopeParent = parent;

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
        // pass we look only for literal ints, and if we find none, issue a warning and choose zero. - buildExprValue()
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
            if (call->arguments != nullptr && call->arguments->nodeType == parse::NodeType::kBlock) {
                const auto condBlockNode = reinterpret_cast<const parse::BlockNode*>(call->arguments.get());
                auto condBlock = buildBlock(condBlockNode, block);
                if (condBlockNode->next != nullptr && condBlockNode->next->nodeType == parse::NodeType::kBlock) {
                    const auto loopBlockNode = reinterpret_cast<const parse::BlockNode*>(condBlockNode->next.get());
                    auto loopBlock = buildBlock(loopBlockNode, block);
                    // Branch into the conditional block from this current containing block.
                    block->exits.emplace_back(condBlock.get());
                    // Loop Block branches back to conditional block.
                    loopBlock->exits.emplace_back(condBlock.get());
                    // Conditional block gets a conditional branch and both exits
                    condBlock->instructions.emplace_back(HIR(Opcode::kBranchIf, { ValueRef(condBlock.get()) }));
                    // For scope, both new blocks are added into current block as children.
                    block->scopeChildren.emplace_back(std::move(condBlock));
                    block->scopeChildren.emplace_back(std::move(loopBlock));
                    if (call->next) {
                        // Create a new block for continued code generation at this level.
                        auto contBlock = std::make_unique<Block>(m_blockSerial);
                        ++m_blockSerial;
                        Block* nextBlock = contBlock.get();
                        contBlock->scopeParent = block;
                        block->scopeChildren.emplace_back(std::move(contBlock));
                        block->exits.emplace_back(nextBlock);
                        // Modify recursion variables to continue adding code to the new Block
                        node = call->next.get();
                        block = nextBlock;
                    }
                }
            } else {
                spdlog::error("while statement without Block args, time for dispatch");
            }
        }
    } break;

    case hadron::parse::kBinopCall: {
        const auto binop = reinterpret_cast<const parse::BinopCallNode*>(node);
        if (binop->selector == kLessThanHash) {
            std::vector<HIR::Operand> operands;
            operands.emplace_back(ValueRef(block));
            if (binop->leftHand->nodeType == parse::NodeType::kName) {
                const auto nameNode = reinterpret_cast<const parse::NameNode*>(binop->leftHand.get());
                Block* nameBlock = block->findContainingScope(nameNode->nameHash);
                if (nameBlock) {
                    operands.emplace_back(ValueRef(nameBlock, nameNode->nameHash, nameNode->name));
                } else {
                    spdlog::warn("Could not find reference for name {}", std::string(nameNode->name.data(),
                        nameNode->name.size()));
                }
            }
            if (binop->rightHand->nodeType == parse::NodeType::kLiteral) {
                const auto literal = reinterpret_cast<const parse::LiteralNode*>(binop->rightHand.get());
                operands.emplace_back(literal->value);
            }
            block->instructions.emplace_back(HIR(kLessThanI32, std::move(operands)));
        }
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

