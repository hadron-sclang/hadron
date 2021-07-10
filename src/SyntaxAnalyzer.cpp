#include "hadron/SyntaxAnalyzer.hpp"

#include "hadron/Lexer.hpp"
#include "hadron/Parser.hpp"
#include "Keywords.hpp"

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

void SyntaxAnalyzer::assignVirtualRegisters() {
    std::unordered_map<Hash, int> activeRegisters;
    std::unordered_set<int> freeRegisters;
    if (m_ast->astType == ast::ASTType::kBlock) {
        auto block = reinterpret_cast<ast::BlockAST*>(m_ast.get());
        for (auto statement = block->statements.begin(); statement != block->statements.end(); ++statement) {
            assignRegistersToBlock(statement->get(), block, activeRegisters, freeRegisters, statement);
        }
    }
}

std::unique_ptr<ast::BlockAST> SyntaxAnalyzer::buildBlock(const Parser* parser, const parse::BlockNode* blockNode,
        ast::BlockAST* parent) {
    auto block = std::make_unique<ast::BlockAST>(parent);

    // Insert special return value sentinel variable.
    block->variables.emplace(std::make_pair(kCaretHash, ast::Value("^")));
    block->variables.emplace(std::make_pair(kCaretAddressHash, ast::Value("_addr_^")));

    // TODO: arguments
    if (blockNode->variables) {
        fillAST(parser, blockNode->variables.get(), block.get(), &(block->statements));
    }
    if (blockNode->body) {
        fillAST(parser, blockNode->body.get(), block.get(), &(block->statements));
    }

    // Transform last statment in block into an assignment to the return sentinel variable, if it isn't already.
    bool hasReturn = false;
    if (block->statements.size()) {
        if (block->statements.back()->astType == ast::ASTType::kAssign) {
            auto lastAssign = reinterpret_cast<ast::AssignAST*>(block->statements.back().get());
            hasReturn = (lastAssign->target->nameHash == kCaretHash);
        }
        if (!hasReturn) {
            auto assign = std::make_unique<ast::AssignAST>();
            assign->value = std::move(block->statements.back());
            assign->target = findValue(kCaretHash, block.get(), true);
            assign->target->valueType = assign->value->valueType;
            assign->valueType = assign->target->valueType;
            block->statements.back() = std::move(assign);
        }
    }

    auto store = std::make_unique<ast::SlotStoreAST>();
    // emplace() will not overwrite existing values.
    store->slotAddress = findValue(kCaretAddressHash, block.get(), true);
    store->storeValue = findValue(kCaretHash, block.get(), false);
    // Always append a Return statement returning the sentinel return value.
    block->statements.emplace_back(std::make_unique<ast::ReturnAST>(std::move(store)));
    return block;
}

std::unique_ptr<ast::InlineBlockAST> SyntaxAnalyzer::buildInlineBlock(const Parser* parser,
        const parse::BlockNode* blockNode, ast::BlockAST* parent) {
    auto inlineBlock = std::make_unique<ast::InlineBlockAST>();

    if (blockNode->variables) {
        fillAST(parser, blockNode->variables.get(), parent, &(inlineBlock->statements));
    }
    if (blockNode->body) {
        fillAST(parser, blockNode->body.get(), parent, &(inlineBlock->statements));
    }

    return inlineBlock;
}

std::unique_ptr<ast::ClassAST> SyntaxAnalyzer::buildClass(const Parser* parser, const parse::ClassNode* classNode) {
    auto classAST = std::make_unique<ast::ClassAST>();
    auto classToken = parser->tokens()[classNode->tokenIndex];
    classAST->nameHash = classToken.hash;
    classAST->name = std::string(classToken.range.data(), classToken.range.size());
    if (classNode->superClassNameIndex) {
        auto superClassToken = parser->tokens()[classNode->superClassNameIndex.value()];
        classAST->superClassHash = superClassToken.hash;
    } else {
        classAST->superClassHash = kObjectHash;
    }
    const parse::VarListNode* varList = classNode->variables.get();
    while (varList) {
        auto varListToken = parser->tokens()[varList->tokenIndex];
        if (varListToken.hash == kVarHash) {
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                auto varNameToken = parser->tokens()[varDef->tokenIndex];
                classAST->variables.emplace(std::make_pair(varNameToken.hash, ast::Value(std::string(
                        varNameToken.range.data(), varNameToken.range.size()))));
                if (varDef->next) {
                    if (varDef->next->nodeType == parse::NodeType::kVarDef) {
                        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
                    }
                }
            }
        } else if (varListToken.hash == kClassVarHash) {
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                auto varNameToken = parser->tokens()[varDef->tokenIndex];
                classAST->classVariables.emplace(std::make_pair(varNameToken.hash, ast::Value(std::string(
                        varNameToken.range.data(), varNameToken.range.size()))));
                if (varDef->next) {
                    if (varDef->next->nodeType == parse::NodeType::kVarDef) {
                        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
                    }
                }
            }
        } else if (varListToken.hash == kConstHash) {
            const parse::VarDefNode* varDef = varList->definitions.get();
            while (varDef) {
                auto varNameToken = parser->tokens()[varDef->tokenIndex];
                classAST->constants.emplace(std::make_pair(varNameToken.hash, varNameToken.value));
                if (varDef->next) {
                    if (varDef->next->nodeType == parse::NodeType::kVarDef) {
                        varDef = reinterpret_cast<const parse::VarDefNode*>(varDef->next.get());
                    }
                }
            }
        } else {
            spdlog::error("Unexpected class variable token.");
        }
        if (varList->next) {
            if (varList->next->nodeType == parse::NodeType::kVarList) {
                varList = reinterpret_cast<const parse::VarListNode*>(varList->next.get());
            }
        }
    }

    return classAST;
}

// Should clarify that |block| is for finding variables only..
void SyntaxAnalyzer::fillAST(const Parser* parser, const parse::Node* parseNode, ast::BlockAST* block,
        std::list<std::unique_ptr<ast::AST>>* ast) {
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
        auto assign = std::make_unique<ast::AssignAST>();
        if (varDefNode->initialValue) {
            assign->value = buildExprTree(parser, varDefNode->initialValue.get(), block);
        } else {
            // Initialize variables to nil if they don't have a specified initial value.
            assign->value = std::make_unique<ast::ConstantAST>(Literal(Type::kNil));
        }
        assign->target = findValue(name.hash, block, true);
        // Propagate type from value to both target and the assignment statement.
        assign->target->valueType = assign->value->valueType;
        assign->valueType = assign->value->valueType;
        ast->emplace_back(std::move(assign));
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
        return buildBinop(parser, binopNode, block);
    } break;

    case parse::NodeType::kCall: {
        const auto callNode = reinterpret_cast<const parse::CallNode*>(parseNode);
        return buildCall(parser, callNode, block);
    } break;

    case parse::NodeType::kAssign: {
        const auto assignNode = reinterpret_cast<const parse::AssignNode*>(parseNode);
        auto assign = std::make_unique<ast::AssignAST>();
        auto nameToken = parser->tokens()[assignNode->name->tokenIndex];
        assign->value = buildExprTree(parser, assignNode->value.get(), block);
        assign->target = findValue(nameToken.hash, block, true);
        assign->target->valueType = assign->value->valueType;
        assign->valueType = assign->value->valueType;
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
        // NOTE that inlining these blocks here only works if the variables declared have unique names within the
        // scope of the parent block - maybe there's a boolean function suitableForInlining() or something that
        // could check that. Inlining should probably be done in a subsequent pass to allow for other inlined blocks
        // with live and dead variables to not potentially interfere with inline this block
        auto whileAST = std::make_unique<ast::WhileAST>();
        const parse::BlockNode* actionNode = nullptr;
        // target can contain the condition argument, or it can be the first argument.
        if (callNode->target) {
            if (callNode->target->nodeType == parse::NodeType::kBlock) {
                whileAST->condition = buildInlineBlock(parser, reinterpret_cast<const parse::BlockNode*>(
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
                whileAST->condition = buildInlineBlock(parser, reinterpret_cast<const parse::BlockNode*>(
                    callNode->arguments.get()), block);
                if (callNode->arguments->next && callNode->arguments->next->nodeType == parse::NodeType::kBlock) {
                    actionNode = reinterpret_cast<const parse::BlockNode*>(callNode->arguments->next.get());
                }
            } else {
                // same error condition as before..
            }
        }
        if (actionNode) {
            whileAST->action = buildInlineBlock(parser, actionNode, block);
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

// TODO: AST construction with this kind of lowering should always produce 3-address operations. Both has Lightning a //
// 3-address IR, and so the tree should reflect that. Perhaps a subsequent pass on the tree could
// rewrite it to 3-address form. - Calculate->left should always be a Value in this form.

// also a subsequent pass for block inlining.
std::unique_ptr<ast::AST> SyntaxAnalyzer::buildBinop(const Parser* parser, const parse::BinopCallNode* binopNode,
        ast::BlockAST* block) {
    // Type of both operands really matters, so check both sides for type.
    auto left = buildExprTree(parser, binopNode->leftHand.get(), block);
    auto right = buildExprTree(parser, binopNode->rightHand.get(), block);
    auto binopToken = parser->tokens()[binopNode->tokenIndex];

    if ((left->valueType & (Type::kInteger | Type::kFloat)) && (right->valueType & (Type::kInteger | Type::kFloat))) {
        switch (binopToken.hash) {
        case kAddHash:
        case kDivideHash:
        case kModuloHash:
        case kMultiplyHash:
        case kSubtractHash: {
            auto calc = std::make_unique<ast::CalculateAST>(binopToken.hash);
            if (left->valueType == Type::kFloat || right->valueType == Type::kFloat) {
                calc->valueType = Type::kFloat;
            } else {
                calc->valueType = Type::kInteger;
            }
            calc->left = std::move(left);
            calc->right = std::move(right);
            return calc;
        } break;

        case kLessThanHash:
        case kEqualToHash:
        case kExactlyEqualToHash:
        case kGreaterThanHash:
        case kGreaterThanOrEqualToHash:
        case kLessThanOrEqualToHash:
        case kNotEqualToHash:
        case kNotExactlyEqualToHash: {
            auto calc = std::make_unique<ast::CalculateAST>(binopToken.hash);
            calc->left = std::move(left);
            calc->right = std::move(right);
            calc->valueType = Type::kBoolean;
            return calc;
        } break;

        default:
            break;
        }
    }

    // Types or hash didn't match any lowering candidates, so create a generic dispatch node.
    auto dispatch = std::make_unique<ast::DispatchAST>();
    dispatch->selectorHash = binopToken.hash;
    dispatch->selector = std::string(binopToken.range.data(), binopToken.range.size());
    dispatch->arguments.emplace_back(std::move(left));
    dispatch->arguments.emplace_back(std::move(right));
    return dispatch;
}

std::unique_ptr<ast::ValueAST> SyntaxAnalyzer::findValue(Hash nameHash, ast::BlockAST* block, bool isWrite) {
    ast::BlockAST* searchBlock = block;
    while (searchBlock) {
        auto nameEntry = searchBlock->variables.find(nameHash);
        if (nameEntry != searchBlock->variables.end()) {
            auto value = std::make_unique<ast::ValueAST>(nameHash, searchBlock);
            value->isWrite = isWrite;
            // TODO: There should always be at least one write to this variable before reading.
            // On reads we propagate type from previous reference.
            auto lastRef = nameEntry->second.references.back();
            if (!isWrite) {
                value->valueType = lastRef->valueType;
            }
            if (nameEntry->second.references.size()) {
                lastRef->isLastReference = false;
            }
            nameEntry->second.references.emplace(nameEntry->second.references.end(), value.get());
            return value;
        }
        searchBlock = searchBlock->parent;
    }

    return nullptr;
}

void SyntaxAnalyzer::assignRegistersToBlock(ast::AST* ast, ast::BlockAST* block,
        std::unordered_map<Hash, int>& activeRegisters, std::unordered_set<int>& freeRegisters,
        std::list<std::unique_ptr<ast::AST>>::iterator currentStatement) {
    switch (ast->astType) {
    case ast::ASTType::kCalculate: {
        auto calculate = reinterpret_cast<ast::CalculateAST*>(ast);
        assignRegistersToBlock(calculate->left.get(), block, activeRegisters, freeRegisters, currentStatement);
        assignRegistersToBlock(calculate->right.get(), block, activeRegisters, freeRegisters, currentStatement);
    } break;

    case ast::ASTType::kBlock:
        // TODO
        break;

    case ast::ASTType::kInlineBlock: {
        auto inlineBlock = reinterpret_cast<ast::InlineBlockAST*>(ast);
        for (auto& statement : inlineBlock->statements) {
            assignRegistersToBlock(statement.get(), block, activeRegisters, freeRegisters, currentStatement);
        }
    } break;

    case ast::ASTType::kValue: {
        auto value = reinterpret_cast<ast::ValueAST*>(ast);
        // Look for value in the active Registers map, if present the register is already assigned.
        auto activeReg = activeRegisters.find(value->nameHash);
        if (activeReg == activeRegisters.end()) {
            int number;
            // Use an already allocated register if available.
            if (freeRegisters.size()) {
                number = *(freeRegisters.begin());
                freeRegisters.erase(freeRegisters.begin());
            } else {
                // Increment the number of needed virtual registers and immediately allocate to this value.
                number = block->numberOfVirtualRegisters;
                ++block->numberOfVirtualRegisters;
            }
            value->registerNumber = number;
            activeRegisters.emplace(std::make_pair(value->nameHash, number));
            // Prepend register mapping statement
            block->statements.emplace(currentStatement, std::make_unique<ast::AliasRegisterAST>(number,
                value->nameHash));
        } else {
            value->registerNumber = activeReg->second;
        }
        // If this is the last reference to this Value we can free up the register now. This means that statements may
        // re-use virtual registers internally, so any tracking of name => register mapping will need to account for
        // potential multiple mappings at once.
        if (value->isLastReference) {
            activeRegisters.erase(value->nameHash);
            freeRegisters.emplace(value->registerNumber);
            // Append register unmapping node.
            auto nextStatement = currentStatement;
            ++nextStatement;
            block->statements.emplace(nextStatement, std::make_unique<ast::UnaliasRegisterAST>(value->registerNumber,
                value->nameHash));
        }
    } break;

    case ast::ASTType::kReturn: {
        auto returnAST = reinterpret_cast<ast::ReturnAST*>(ast);
        assignRegistersToBlock(returnAST->value.get(), block, activeRegisters, freeRegisters, currentStatement);
    } break;

    case ast::ASTType::kAssign: {
        auto assign = reinterpret_cast<ast::AssignAST*>(ast);
        assignRegistersToBlock(assign->value.get(), block, activeRegisters, freeRegisters, currentStatement);
        assignRegistersToBlock(assign->target.get(), block, activeRegisters, freeRegisters, currentStatement);
    } break;

    case ast::ASTType::kConstant:
        // Constants are no-ops. Good constant propagation through values in a pass before register assignment will
        // likely help relieve register pressure on the allocator, as constant values can be dropped from registers
        // without spilling out to memory.
        break;

    case ast::ASTType::kWhile: {
        auto whileAST = reinterpret_cast<ast::WhileAST*>(ast);
        assignRegistersToBlock(whileAST->condition.get(), block, activeRegisters, freeRegisters, currentStatement);
        assignRegistersToBlock(whileAST->action.get(), block, activeRegisters, freeRegisters, currentStatement);
    } break;

    case ast::ASTType::kDispatch: {
        // TODO call stack preservation
    } break;

    case ast::ASTType::kClass: {
        // TODO: Weird internal error.
    } break;

    case ast::ASTType::kSlotLoad: {
        // TODO
    } break;

    case ast::ASTType::kSlotStore: {
        auto store = reinterpret_cast<ast::SlotStoreAST*>(ast);
        assignRegistersToBlock(store->slotAddress.get(), block, activeRegisters, freeRegisters, currentStatement);
        assignRegistersToBlock(store->storeValue.get(), block, activeRegisters, freeRegisters, currentStatement);
    } break;

    case ast::ASTType::kUnaliasRegister:
        break;

    // weird to encounter these because it's this function's job to insert them
    case ast::ASTType::kAliasRegister:
        break;
    }
}

} // namespace hadron