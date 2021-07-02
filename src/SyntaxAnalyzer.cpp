#include "SyntaxAnalyzer.hpp"

#include "Keywords.hpp"
#include "Parser.hpp"

#include "spdlog/spdlog.h"

namespace hadron {

SyntaxAnalyzer::SyntaxAnalyzer(std::shared_ptr<ErrorReporter> errorReporter): m_errorReporter(errorReporter) {}

bool SyntaxAnalyzer::buildAST(const Parser* parser) {
    const parse::Node* root = parser->root();
    if (root->nodeType == parse::NodeType::kBlock) {
        auto blockNode = reinterpret_cast<const parse::BlockNode*>(root);
        m_ast = buildBlock(parser, blockNode, nullptr);
    }

    return m_ast != nullptr;
}

std::unique_ptr<ast::BlockAST> SyntaxAnalyzer::buildBlock(const Parser* parser, const parse::BlockNode* blockNode,
        ast::BlockAST* parent) {
    std::unique_ptr<ast::BlockAST> block = std::make_unique<ast::BlockAST>(parent);

    // TODO: arguments
    if (blockNode->variables) {
        fillAST(parser, blockNode->variables.get(), block.get(), &(block->statements));
    }
    if (blockNode->body) {
        fillAST(parser, blockNode->body.get(), block.get(), &(block->statements));
    }
    return block;
}

void SyntaxAnalyzer::fillAST(const Parser* parser, const parse::Node* parseNode, ast::BlockAST* block,
        std::vector<std::unique_ptr<ast::AST>>* ast) {
    switch (parseNode->nodeType) {
    case parse::NodeType::kVarList: {
        const auto varListNode = reinterpret_cast<const parse::VarListNode*>(parseNode);
        if (varListNode->definitions) {
            fillAST(parser, varListNode->definitions.get(), block, ast);
        }
    } break;

    case parse::NodeType::kVarDef: {
        const auto varDefNode = reinterpret_cast<const parse::VarDefNode*>(parseNode);
        auto name = parser->tokens()[varDefNode->tokenIndex];
        // LSC allows hiding of variables outside of local scope, so we do not search up the block tree to check
        // if there is a similarly-named variable up there, though we might want to do so in the future to issue
        // a warning.
        if (block->variables.find(name.hash) != block->variables.end()) {
            // TODO:: error reporting for variable redefinition
        }
        block->variables.emplace(std::make_pair(name.hash,
            ast::Value(std::string(name.range.data(), name.range.size()))));
        if (varDefNode->initialValue) {
            auto assign = std::make_unique<ast::AssignAST>();
            assign->target = std::make_unique<ast::ValueAST>(name.hash, block);
            assign->value = buildExprTree(parser, varDefNode->initialValue.get(), block);
            ast->emplace_back(std::move(assign));
        }
    } break;

    case parse::NodeType::kExprSeq: {
        const auto exprSeqNode = reinterpret_cast<const parse::ExprSeqNode*>(parseNode);
        if (exprSeqNode->expr) {
            fillAST(parser, exprSeqNode->expr.get(), block, ast);
        }
    } break;

    case parse::NodeType::kAssign:
    case parse::NodeType::kBinopCall:
    case parse::NodeType::kCall:
    case parse::NodeType::kLiteral:
    case parse::NodeType::kName:
        ast->emplace_back(buildExprTree(parser, parseNode, block));
        break;

    default:
        break;
    }

    if (parseNode->next) {
        fillAST(parser, parseNode->next.get(), block, ast);
    }
}

std::unique_ptr<ast::AST> SyntaxAnalyzer::buildExprTree(const Parser* parser, const parse::Node* parseNode,
        ast::BlockAST* block) {
    switch (parseNode->nodeType) {
    case parse::NodeType::kLiteral: {
        const auto literalNode = reinterpret_cast<const parse::LiteralNode*>(parseNode);
        return std::make_unique<ast::ConstantAST>(literalNode->value);
    } break;

    case parse::NodeType::kName: {
        const auto nameNode = reinterpret_cast<const parse::NameNode*>(parseNode);
        auto name = findValue(parser->tokens()[nameNode->tokenIndex].hash, block, false);
        if (!name) {
            // TODO: name not found error reporting
        }
        return name;
    } break;

    case parse::NodeType::kBinopCall: {
        const auto binopNode = reinterpret_cast<const parse::BinopCallNode*>(parseNode);
        auto token = parser->tokens()[binopNode->tokenIndex];
        auto binop = std::make_unique<ast::BinopAST>(token.hash, std::string(token.range.data(), token.range.size()));
        binop->left = buildExprTree(parser, binopNode->leftHand.get(), block);
        binop->right = buildExprTree(parser, binopNode->rightHand.get(), block);
        return binop;
    } break;

    case parse::NodeType::kCall: {
        const auto callNode = reinterpret_cast<const parse::CallNode*>(parseNode);
        return buildCall(parser, callNode, block);
    } break;

    case parse::NodeType::kAssign: {
        const auto assignNode = reinterpret_cast<const parse::AssignNode*>(parseNode);
        auto assign = std::make_unique<ast::AssignAST>();
        auto nameToken = parser->tokens()[assignNode->name->tokenIndex];
        assign->target = findValue(nameToken.hash, block, true);
        assign->value = buildExprTree(parser, assignNode->value.get(), block);
        return assign;
    } break;

    default:
        break;
    }

    return nullptr;
}

std::unique_ptr<ast::AST> SyntaxAnalyzer::buildCall(const Parser* parser, const parse::CallNode* callNode,
        ast::BlockAST* block) {
    auto callToken = parser->tokens()[callNode->tokenIndex];
    // Might make sense to build a more strict call => dispatchAST transformation first, then after type analysis pass
    // could more easily pattern match the dispatches to lower-level stuff
    switch (callToken.hash) {
    case kWhileHash: {
        auto whileAST = std::make_unique<ast::WhileAST>();
        const parse::BlockNode* actionNode = nullptr;
        // target can contain the condition argument, or it can be the first argument.
        if (callNode->target) {
            if (callNode->target->nodeType == parse::NodeType::kBlock) {
                whileAST->condition = buildBlock(parser, reinterpret_cast<const parse::BlockNode*>(
                    callNode->target.get()), block);
                if (callNode->arguments && callNode->arguments->nodeType == parse::NodeType::kBlock) {
                    actionNode = reinterpret_cast<const parse::BlockNode*>(callNode->arguments.get());
                }
            } else {
                // TODO: unexpected type condition, expecting Block argument for a while call, maybe this fails back to
                // a generic dispatch call to this object?
            }
        } else {
            if (callNode->arguments && callNode->arguments->nodeType == parse::NodeType::kBlock) {
                whileAST->condition = buildBlock(parser, reinterpret_cast<const parse::BlockNode*>(
                    callNode->arguments.get()), block);
                if (callNode->arguments->next && callNode->arguments->next->nodeType == parse::NodeType::kBlock) {
                    actionNode = reinterpret_cast<const parse::BlockNode*>(callNode->arguments->next.get());
                }
            } else {
                // same error condition as before..
            }
        }
        if (actionNode) {
            whileAST->action = buildBlock(parser, actionNode, block);
            return whileAST;
        }
    } break;
    }

    auto dispatch = std::make_unique<ast::DispatchAST>();
    dispatch->selectorHash = callToken.hash;
    dispatch->selector = std::string(callToken.range.data(), callToken.range.size());
    if (callNode->target) {
        fillAST(parser, callNode->target.get(), block, &(dispatch->arguments));
    }
    if (callNode->arguments) {
        fillAST(parser, callNode->arguments.get(), block, &(dispatch->arguments));
    }
    // TODO: keyword arguments
    return dispatch;
}

std::unique_ptr<ast::ValueAST> SyntaxAnalyzer::findValue(uint64_t nameHash, ast::BlockAST* block, bool addReference) {
    ast::BlockAST* searchBlock = block;
    while (searchBlock) {
        auto nameEntry = searchBlock->variables.find(nameHash);
        if (nameEntry != searchBlock->variables.end()) {
            auto value = std::make_unique<ast::ValueAST>(nameHash, searchBlock);
            if (addReference) {
                nameEntry->second.revisions.emplace_back(value.get());
            }
            value->revision = nameEntry->second.revisions.size();
            return value;
        }
        searchBlock = searchBlock->parent;
    }

    return nullptr;
}

} // namespace hadron